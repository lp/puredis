#include "m_pd.h"
#include <hiredis.h>
#include <async.h>
#include <stdlib.h>
#include <string.h>

/* gcc -ansi -Wall -O2 -fPIC -bundle -undefined suppress -flat_namespace -arch i386 -I/Applications/Pd-extended.app/Contents/Resources/include -I./hiredis/includes ./hiredis/libhiredis.a -o puredis.pd_darwin puredis.c */

#define MAX_ARRAY_SIZE 512

static t_class *puredis_class;
static t_class *apuredis_class;

typedef struct _puredis {
    /*char * name;*/
    t_object x_obj;
    int async;
    int qcount;
    t_outlet *q_out;
    
    redisContext * redis;
    
    char * r_host;
    int r_port;
    int out_count;
    t_atom out[MAX_ARRAY_SIZE];
    
} t_puredis;

static void freeVectorAndLengths(int argc, char ** vector, size_t * lengths)
{
    int i;
    for (i = 0; i < argc; i++) free(vector[i]);
    free(vector);
    free(lengths);
}

void prepareOutList(t_puredis *x, redisReply * reply) {
    if (reply->type == REDIS_REPLY_ERROR) {
        SETSYMBOL(&x->out[x->out_count],gensym(reply->str));
        x->out_count++;
    } else if (reply->type == REDIS_REPLY_STATUS) {
        SETSYMBOL(&x->out[x->out_count],gensym(reply->str));
        x->out_count++;
    } else if (reply->type == REDIS_REPLY_STRING) {
        SETSYMBOL(&x->out[x->out_count],gensym(reply->str));
        x->out_count++;
    } else if (reply->type == REDIS_REPLY_ARRAY) {
        int i;
        for (i = 0; i < (int)reply->elements; i++) {
            prepareOutList(x,reply->element[i]);
        }
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        SETFLOAT(&x->out[x->out_count],reply->integer);
        x->out_count++;
    }
}

void parseReply(t_puredis *x, redisReply * reply)
{
    if (reply->type == REDIS_REPLY_ERROR) {
        outlet_symbol(x->x_obj.ob_outlet, gensym(reply->str));
    } else if (reply->type == REDIS_REPLY_STATUS) {
        outlet_symbol(x->x_obj.ob_outlet, gensym(reply->str));
    } else if (reply->type == REDIS_REPLY_STRING) {
        outlet_symbol(x->x_obj.ob_outlet, gensym(reply->str));
    } else if (reply->type == REDIS_REPLY_ARRAY) {
        x->out_count = 0;
        prepareOutList(x,reply);
        outlet_list(x->x_obj.ob_outlet, &s_list, x->out_count, &x->out[0]);
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        t_atom value;
        SETFLOAT(&value, reply->integer);
        outlet_float(x->x_obj.ob_outlet, atom_getfloat(&value));
    }
    freeReplyObject(reply);
}

static void postCommandSync(t_puredis * x, int argc, char ** vector, size_t * lengths)
{
    redisReply * reply = redisCommandArgv(x->redis, argc, (const char**)vector, (const size_t *)lengths);
    parseReply(x,reply);
    freeVectorAndLengths(argc, vector, lengths);
}

static void apuredis_q_out(t_puredis * x)
{
    t_atom value;
    SETFLOAT(&value, x->qcount);
    outlet_float(x->q_out, atom_getfloat(&value));
}

void apuredis_yield(t_puredis * x)
{
    if (x->qcount > 0) {
        void * tmpreply = NULL;
        if ( redisGetReply(x->redis, &tmpreply) == REDIS_ERR) return;
        if (tmpreply == NULL) {
            int wdone = 0;
            if (redisBufferWrite(x->redis,&wdone) == REDIS_ERR) return;
            
            if (redisBufferRead(x->redis) == REDIS_ERR)
                return;
            if (redisGetReplyFromReader(x->redis,&tmpreply) == REDIS_ERR)
                return;
            
            if (tmpreply != NULL) {
                x->qcount--;
                redisReply * reply = (redisReply*)tmpreply;
                parseReply(x, reply);
            }
        } else {
            x->qcount--;
            redisReply * reply = (redisReply*)tmpreply;
            parseReply(x, reply);
        }
    }
    apuredis_q_out(x);
}

static void postCommandAsync(t_puredis * x, int argc, char ** vector, size_t * lengths)
{
    redisAppendCommandArgv(x->redis, argc, (const char**)vector, (const size_t *)lengths);
    freeVectorAndLengths(argc, vector, lengths);
    x->qcount++;
    apuredis_q_out(x);
}

void redis_command(t_puredis *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc < 1) {
        post("puredis: wrong command"); return;
    }
    
    int i;
    char ** vector = NULL;
    size_t * lengths = NULL;
    
    if (((vector = malloc(argc*sizeof(char*))) == NULL) || ((lengths = malloc(argc*sizeof(size_t))) == NULL)) {
        post("puredis: can not proceed!!  Memory Error!"); return;
    }
    
    for (i = 0; i < argc; i++) {
        char cmdpart[256];
        atom_string(argv+i, cmdpart, 256);
        
        if ((vector[i] = malloc(strlen(cmdpart)+1)) == NULL) {
            post("puredis: can not proceed!!  Memory Error!"); return;
        }
        
        strcpy(vector[i], (char*)cmdpart);
        lengths[i] = strlen(vector[i]);
    }
    if (x->async) {
        postCommandAsync(x, argc, vector, lengths);
    } else {
        postCommandSync(x, argc, vector, lengths);
    }
}

void *redis_new(t_symbol *s, int argc, t_atom *argv)
{
    t_puredis *x = NULL;
    if (s == gensym("apuredis")) {
        /* x->name = "apuredis"; */
        x = (t_puredis*)pd_new(apuredis_class);
        x->async = 1; x->qcount = 0;
    } else {
        /* x->name = "puredis"; */
        x = (t_puredis*)pd_new(puredis_class);
        x->async = 0;
    }
    
    int port = 6379;
    char host[16] = "127.0.0.1";
    switch(argc){
        default:
            break;
        case 2:
            port = (int)atom_getint(argv+1);
        case 1:
            atom_string(argv, host, 16);
            break;
        case 0:
            break;
    }
    
    x->r_host = (char*)host;
    x->r_port = port;
    
    outlet_new(&x->x_obj, NULL);
    if (x->async) {
        x->redis = redisConnectNonBlock(x->r_host,x->r_port);
        x->q_out = outlet_new(&x->x_obj, &s_float);
    } else {
        x->redis = redisConnect(x->r_host,x->r_port);
    }
    if (x->redis->err) {
        post("could not connect to redis...");
        return NULL;
    }
    
    post("connected to redis host: %s port: %u", x->r_host, x->r_port);
    
    return (void*)x;
}

void apuredis_bang(t_puredis *x) {
    apuredis_yield(x);
}

void redis_free(t_puredis *x)
{
    redisFree(x->redis);
}

static void setup_puredis(void)
{
    puredis_class = class_new(gensym("puredis"),
        (t_newmethod)redis_new,
        (t_method)redis_free,
        sizeof(t_puredis),
        CLASS_DEFAULT,
        A_GIMME, 0);
    
    class_addmethod(puredis_class,
        (t_method)redis_command, gensym("command"),
        A_GIMME, 0);
    class_sethelpsymbol(puredis_class, gensym("puredis-help"));
}

static void setup_apuredis(void)
{
    apuredis_class = class_new(gensym("apuredis"),
        (t_newmethod)redis_new,
        (t_method)redis_free,
        sizeof(t_puredis),
        CLASS_DEFAULT,
        A_GIMME, 0);
    
    class_addbang(apuredis_class,apuredis_bang);
    class_addmethod(apuredis_class,
        (t_method)redis_command, gensym("command"),
        A_GIMME, 0);
    class_sethelpsymbol(apuredis_class, gensym("puredis-help"));
}

void puredis_setup(void) {
    setup_puredis();
    setup_apuredis();
}
