#include "m_pd.h"
#include <hiredis.h>
#include <stdlib.h>
#include <string.h>

/* gcc -ansi -Wall -O2 -fPIC -bundle -undefined suppress -flat_namespace -arch i386 -I/Applications/Pd-extended.app/Contents/Resources/include -I./hiredis/includes ./hiredis/libhiredis.a -o puredis.pd_darwin puredis.c */

#define MAX_ARRAY_SIZE 256

static t_class *puredis_class;

typedef struct _puredis {
    t_object x_obj;
    redisContext * redis;
    char * r_host;
    int out_count;
    t_atom out[MAX_ARRAY_SIZE];
    int r_port;
} t_puredis;

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

void puredis_command(t_puredis *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc < 1) {
        post("puredis: wrong command"); return;
    }
    
    char ** vector = NULL;
    size_t * lengths = NULL;
    int i;
    
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
    
    redisReply * reply = redisCommandArgv(x->redis, argc, (const char**)vector, lengths);
    
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
    for (i = 0; i < argc; i++) free(vector[i]);
    free(vector);
    free(lengths);
}

void *puredis_new(t_symbol *s, int argc, t_atom *argv)
{
    t_puredis *x = (t_puredis*)pd_new(puredis_class);
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
    redisContext *c = redisConnect(x->r_host,x->r_port);
    if (c->err) {
        post("could not connect to redis...");
        return NULL;
    }
    post("connected to redis host: %s port: %u", x->r_host, x->r_port);
    x->redis = c;
    
    outlet_new(&x->x_obj, NULL);
    return (void*)x;
}

void puredis_free(t_puredis *x)
{
    redisFree(x->redis);
}

void puredis_setup(void) {
    puredis_class = class_new(gensym("puredis"),
        (t_newmethod)puredis_new,
        (t_method)puredis_free,
        sizeof(t_puredis),
        CLASS_DEFAULT,
        A_GIMME, 0);
    
    class_addmethod(puredis_class,
        (t_method)puredis_command, gensym("command"),
        A_GIMME, 0);
    class_sethelpsymbol(puredis_class, gensym("puredis-help"));
}
