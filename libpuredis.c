/*
Copyright (c) 2011 Louis-Philippe Perron

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "m_pd.h"
#include <hiredis.h>
#include <async.h>
#include <stdlib.h>
#include <string.h>
#include <csv.h>

#define PUREDIS_MAJOR 0
#define PUREDIS_MINOR 5
#define PUREDIS_PATCH 3

#define MAX_ARRAY_SIZE 512

/************************************
 * Puredis                          *
 *  Puredata Redis External         *
 *                                  *
 ************************************/
static t_class *puredis_class;

/************************************
 * Apuredis                         *
 *  Async Redis External for PD     *
 *                                  *
 ************************************/
static t_class *apuredis_class;

/************************************
 * Spuredis                         *
 *  Redis Subscriber external for PD*
 *                                  *
 ************************************/
static t_class *spuredis_class;

typedef struct _redis {
    t_object x_obj;
    redisContext * redis;
    
    char * r_host;
    int r_port;
    int out_count;
    t_atom out[MAX_ARRAY_SIZE];
    
    /* async vars */
    int async;
    t_outlet *q_out;
    int async_num;
    int async_prev_num;
    int async_run;
    t_clock  *async_clock;
    
    /* loader vars */
    t_symbol * ltype;
    int lnumload;
    int lnumerror;
    char * lcmd;
    char * lkey;
    int lnew;
    int lcount;
    /* hash loader vars */
    int hargc;
    char ** hheaders;
    int hbufsize;
    int hcount;
    /* zset loader vars */
    int zcount;
    char * zscore;
} t_redis;

/* declarations */
void puredis_setup(void);

/* memory */
static void freeVectorAndLengths(int argc, char ** vector, size_t * lengths);
void redis_free(t_redis *x);

/* general */
void *redis_new(t_symbol *s, int argc, t_atom *argv);
void redis_command(t_redis *x, t_symbol *s, int argc, t_atom *argv);
static void redis_postCommandAsync(t_redis * x, int argc, char ** vector, size_t * lengths);
static void redis_prepareOutList(t_redis *x, redisReply * reply);
static void redis_parseReply(t_redis *x, redisReply * reply);

/* puredis */
static void setup_puredis(void);
static void puredis_postCommandSync(t_redis * x, int argc, char ** vector, size_t * lengths);
static void puredis_csv_postCommand(t_redis * x, int argc, char ** vector, size_t * lengths);
static void puredis_csv_parse(t_redis *x, int argc, void *s, size_t i);
static void puredis_csv_cb1 (void *s, size_t i, void *userdata);
static void puredis_csv_cb2 (int c, void *userdata);
static void puredis_csv_init(t_redis *x);
static void puredis_csv_free(t_redis *x);
void puredis_csv(t_redis *x, t_symbol *s, int argc, t_atom *argv);

/* async redis */
static void setup_apuredis(void);
static void apuredis_yield(t_redis * x);
static void apuredis_q_out(t_redis * x);
static void apuredis_run(t_redis *x);
static void apuredis_schedule(t_redis *x);
void apuredis_bang(t_redis *x);
void apuredis_start(t_redis *x, t_symbol *s);
void apuredis_stop(t_redis *x, t_symbol *s);

/* subscriber redis */
static void setup_spuredis(void);
static void spuredis_run(t_redis *x);
static void spuredis_schedule(t_redis *x);
static void spuredis_manage(t_redis *x, t_symbol *s, int argc);
void spuredis_bang(t_redis *x);
void spuredis_start(t_redis *x, t_symbol *s);
void spuredis_stop(t_redis *x, t_symbol *s);
void spuredis_subscribe(t_redis *x, t_symbol *s, int argc, t_atom *argv);


/* implementation */

/* mandatory PD setup method */
void puredis_setup(void)
{
    setup_puredis();
    setup_apuredis();
    setup_spuredis();
    post("Puredis %i.%i.%i (MIT) 2011 Louis-Philippe Perron <lp@spiralix.org>", PUREDIS_MAJOR, PUREDIS_MINOR, PUREDIS_PATCH);
    post("Puredis: compiled for pd-%d.%d on %s %s", PD_MAJOR_VERSION, PD_MINOR_VERSION, __DATE__, __TIME__);
}

/* memory management methods */

static void freeVectorAndLengths(int argc, char ** vector, size_t * lengths)
{
    int i;
    for (i = 0; i < argc; i++) free(vector[i]);
    free(vector);
    free(lengths);
}

void redis_free(t_redis *x)
{
    redisFree(x->redis);
}

/* common methods */

/* constructor used by Puredis, Apuredis and Spuredis */
void *redis_new(t_symbol *s, int argc, t_atom *argv)
{
    t_redis *x = NULL;
    
    int port = 6379;                /* default port */
    char host[16] = "127.0.0.1";    /* default IP address */
    switch(argc){   /* parsing init arguments */
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
    
    /* class initialisation */
    if (s == gensym("apuredis")) {
        x = (t_redis*)pd_new(apuredis_class);
        x->redis = redisConnectNonBlock((char*)host,port);
        x->async = 1; x->async_num = 0;
        x->async_prev_num = 0; x->async_run = 0;
        x->async_clock = clock_new(x, (t_method)apuredis_run);
    } else if (s == gensym("spuredis")) {
        x = (t_redis*)pd_new(spuredis_class);
        x->redis = redisConnectNonBlock((char*)host,port);
        x->async_clock = clock_new(x, (t_method)spuredis_run);
        x->async_num = 0; x->async_run = 0;
    } else {
        x = (t_redis*)pd_new(puredis_class);
        x->redis = redisConnect((char*)host,port);
        x->async = 0;
    }
    x->r_host = (char*)host;
    x->r_port = port;
    outlet_new(&x->x_obj, NULL);
    if (x->async) {
        x->q_out = outlet_new(&x->x_obj, &s_float);
    }
    
    if (x->redis->err) {
        post("could not connect to redis...");
        return NULL;
    }
    post("Puredis %i.%i.%i connected to redis host: %s port: %u", PUREDIS_MAJOR, PUREDIS_MINOR, PUREDIS_PATCH, x->r_host, x->r_port);
    
    return (void*)x;
}

/* redis command parsing -- Puredis/Apuredis */
void redis_command(t_redis *x, t_symbol *s, int argc, t_atom *argv)
{
    (void)s;
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
        redis_postCommandAsync(x, argc, vector, lengths);
        x->async_num++;
        apuredis_q_out(x);
        apuredis_schedule(x);
    } else {
        puredis_postCommandSync(x, argc, vector, lengths);
    }
}

/* sends command async to Redis */
static void redis_postCommandAsync(t_redis * x, int argc, char ** vector, size_t * lengths)
{
    redisAppendCommandArgv(x->redis, argc, (const char**)vector, (const size_t *)lengths);
    freeVectorAndLengths(argc, vector, lengths);
}

/* recursive redis reply parsing as pd list */
static void redis_prepareOutList(t_redis *x, redisReply * reply)
{
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
            redis_prepareOutList(x,reply->element[i]);
        }
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        SETFLOAT(&x->out[x->out_count],reply->integer);
        x->out_count++;
    } else if (reply->type == REDIS_REPLY_NIL) {
        SETSYMBOL(&x->out[x->out_count],gensym("nil"));
        x->out_count++;
    }
}

/* sends redis reply to outlet */
static void redis_parseReply(t_redis *x, redisReply * reply)
{
    if (reply->type == REDIS_REPLY_ERROR) {
        outlet_symbol(x->x_obj.ob_outlet, gensym(reply->str));
    } else if (reply->type == REDIS_REPLY_STATUS) {
        outlet_symbol(x->x_obj.ob_outlet, gensym(reply->str));
    } else if (reply->type == REDIS_REPLY_STRING) {
        outlet_symbol(x->x_obj.ob_outlet, gensym(reply->str));
    } else if (reply->type == REDIS_REPLY_ARRAY) {
        x->out_count = 0;
        redis_prepareOutList(x,reply);
        outlet_list(x->x_obj.ob_outlet, &s_list, x->out_count, &x->out[0]);
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        t_atom value;
        SETFLOAT(&value, reply->integer);
        outlet_float(x->x_obj.ob_outlet, atom_getfloat(&value));
    } else if (reply->type == REDIS_REPLY_NIL) {
        outlet_symbol(x->x_obj.ob_outlet, gensym("nil"));
    }
    freeReplyObject(reply);
}

/* puredis */

/* puredis setup method */
static void setup_puredis(void)
{
    puredis_class = class_new(gensym("puredis"),
        (t_newmethod)redis_new,
        (t_method)redis_free,
        sizeof(t_redis),
        CLASS_DEFAULT,
        A_GIMME, 0);
    
    class_addmethod(puredis_class,
        (t_method)redis_command, gensym("command"),
        A_GIMME, 0);
    class_addmethod(puredis_class,
        (t_method)puredis_csv, gensym("csv"),
        A_GIMME, 0);
    class_sethelpsymbol(puredis_class, gensym("puredis-help"));
}

/* sends command sync to Redis */
static void puredis_postCommandSync(t_redis * x, int argc, char ** vector, size_t * lengths)
{
    redisReply * reply = redisCommandArgv(x->redis, argc, (const char**)vector, (const size_t *)lengths);
    freeVectorAndLengths(argc, vector, lengths);
    redis_parseReply(x,reply);
}

/* sends command sync to redis for csv data loading */
static void puredis_csv_postCommand(t_redis * x, int argc, char ** vector, size_t * lengths)
{
    redisReply * reply = redisCommandArgv(x->redis, argc, (const char**)vector, (const size_t *)lengths);
    freeVectorAndLengths(argc, vector, lengths);
    x->lnumload++;
    if (reply->type == REDIS_REPLY_ERROR) {
        x->lnumerror++;
        x->lnumload--;
        post("Puredis csv load Redis error: %s", reply->str);
    }
    freeReplyObject(reply);
}

/* parse csv data and loads it in redis */
static void puredis_csv_parse(t_redis *x, int argc, void *s, size_t i)
{
    char ** vector = NULL;
    size_t * lengths = NULL;
    char * item = (char*)s;
    
    if (((vector = malloc(argc*sizeof(char*))) == NULL) || ((lengths = malloc(argc*sizeof(size_t))) == NULL)) {
        post("puredis: can not proceed!!  Memory Error!"); return;
    }
    
    if ((vector[0] = malloc(strlen(x->lcmd)+1)) == NULL) {
        post("puredis: can not proceed!!  Memory Error!"); return;
    }
    strcpy(vector[0], x->lcmd);
    lengths[0] = strlen(vector[0]);
    
    if ((vector[1] = malloc(strlen(x->lkey)+1)) == NULL) {
        post("puredis: can not proceed!!  Memory Error!"); return;
    }
    strcpy(vector[1], x->lkey);
    lengths[1] = strlen(vector[1]);
    
    if (x->ltype == gensym("hash")) {
        if ((vector[2] = malloc(strlen(x->hheaders[x->hcount])+1)) == NULL) {
            post("puredis: can not proceed!!  Memory Error!"); return;
        }
        strcpy(vector[2], x->hheaders[x->hcount]);
        lengths[2] = strlen(vector[2]);
    } else if (x->ltype == gensym("zset")) {
        if ((vector[2] = malloc(strlen(x->zscore)+1)) == NULL) {
            post("puredis: can not proceed!!  Memory Error!"); return;
        }
        strcpy(vector[2], x->zscore);
        lengths[2] = strlen(vector[2]);
    }
    
    if ((vector[argc-1] = malloc(i+1)) == NULL) {
        post("puredis: can not proceed!!  Memory Error!"); return;
    }
    strcpy(vector[argc-1], item);
    lengths[argc-1] = i;
    
    puredis_csv_postCommand(x, argc, vector, lengths);
}

/* libcsv callback for each values in csv data */
static void puredis_csv_cb1 (void *s, size_t i, void *userdata)
{
    t_redis *x = (t_redis *)userdata;
    
    if (x->lnew && x->lkey[0] == '#') return;
    
    if (x->ltype == gensym("hash")) {
        if (x->lcount > 0) {
            if (x->lnew) {
                x->hcount++;
                puredis_csv_parse(x, 4, s,i);
            } else {
                x->lnew = 1;
                x->hcount = 0;
                free(x->lkey);
                if ((x->lkey = malloc(i+1)) == NULL) {
                    post("puredis: can not proceed!!  Memory Error!"); return;
                }
                strcpy(x->lkey, (char*)s);
            }
        } else {
            if ((x->hheaders[x->hcount] = malloc(i+1)) == NULL) {
                post("puredis: can not proceed!!  Memory Error!"); return;
            }
            strcpy(x->hheaders[x->hcount], (char*)s);
            x->hcount++;
            x->hargc++;
            if (x->hbufsize == x->hargc) {
                x->hbufsize += x->hbufsize / 2;
                x->hheaders = realloc(x->hheaders, x->hbufsize * sizeof(char*));
            }
        }
    } else if (x->ltype == gensym("zset")) {
        if (x->lnew) {
            if (x->zcount) {
                puredis_csv_parse(x, 4, s,i);
                x->zcount = 0;
            } else {
                x->zcount = 1;
                free(x->zscore);
                if ((x->zscore = malloc(i+1)) == NULL) {
                    post("puredis: can not proceed!!  Memory Error!"); return;
                }
                strcpy(x->zscore, (char*)s);
            }
        } else {
            x->lnew = 1;
            x->zcount = 0;
            free(x->lkey);
            if ((x->lkey = malloc(i+1)) == NULL) {
                post("puredis: can not proceed!!  Memory Error!"); return;
            }
            strcpy(x->lkey, (char*)s);
        }
    } else {
        if (x->lnew) {
            puredis_csv_parse(x, 3, s,i);
        } else {
            x->lnew = 1;
            
            free(x->lkey);
            if ((x->lkey = malloc(i+1)) == NULL) {
                post("puredis: can not proceed!!  Memory Error!"); return;
            }
            strcpy(x->lkey, (char*)s);
        }
    }
}

/* libcsv callback for each line change in csv data */
static void puredis_csv_cb2 (int c, void *userdata)
{
    (void)c;
    t_redis *x = (t_redis *)userdata;
    x->lnew = 0;
    x->lcount++;
}

/* libcsv csv data parsing initialization */
static void puredis_csv_init(t_redis *x)
{
    x->lcount = 0; x->lnew = 0; x->lnumload = 0; x->lnumerror = 0;
    if ((x->lkey = malloc(8)) == NULL) {
        post("puredis: can not proceed!!  Memory Error!"); return;
    }
    
    char * cmd = NULL;
    if (x->ltype == gensym("string")) {
        cmd = "SET";
    } else if (x->ltype == gensym("list")) {
        cmd = "RPUSH";
    } else if (x->ltype == gensym("set")) {
        cmd = "SADD";
    } else if (x->ltype == gensym("zset")) {
        if ((x->zscore = malloc(8)) == NULL) {
            post("puredis: can not proceed!!  Memory Error!"); return;
        }
        cmd = "ZADD";
    } else if (x->ltype == gensym("hash")) {
        x->hargc = 0;
        x->hcount = 0;
        x->hbufsize = 2;
        x->hheaders = NULL;
        if ((x->hheaders = calloc(x->hbufsize, sizeof(char*))) == NULL) {
            post("puredis: can not proceed!!  Memory Error!"); return;
        }
        cmd = "HSET";
    }
    
    if ((x->lcmd = malloc(strlen(cmd)+1)) == NULL) {
        post("puredis: can not proceed!!  Memory Error!"); return;
    }
    strcpy(x->lcmd, cmd);
}

/* libcsv cleanup */
static void puredis_csv_free(t_redis *x)
{
    free(x->lkey);
    free(x->lcmd);
    if (x->ltype == gensym("hash")) {
        int i;
        for (i = 0; i < x->hargc; i++) free(x->hheaders[i]);
        free(x->hheaders);
    } else if (x->ltype == gensym("zset")) {
        free(x->zscore);
    }
}

/* puredis csv data load method */
void puredis_csv(t_redis *x, t_symbol *s, int argc, t_atom *argv)
{
    (void)s; (void)argc;
    char buf[1024]; size_t i; char filename[256];
    atom_string(argv, filename, 256);
    x->ltype = atom_getsymbol(argv+1);
    
    struct csv_parser p;
    FILE *csvfile = NULL;
    csv_init(&p, 0);
    csv_set_opts(&p, CSV_APPEND_NULL);
    puredis_csv_init(x);
    
    csvfile = fopen((const char*)filename, "rb");
    if (csvfile == NULL) {
        post("Puredis failed to open csv file: %s", csvfile);
        puredis_csv_free(x);
        return;
    }
    
    while ((i=fread(buf, 1, 1024, csvfile)) > 0) {
        if (csv_parse(&p, buf, i, puredis_csv_cb1, puredis_csv_cb2, x) != i) {
            post("Puredis error parsing csv file: %s", csv_strerror(csv_error(&p)));
            fclose(csvfile);
            puredis_csv_free(x);
            return;
        }
    }
    
    csv_fini(&p, puredis_csv_cb1, puredis_csv_cb2, x);
    csv_free(&p);
    puredis_csv_free(x);
    
    t_atom stats[7];
    SETSYMBOL(&stats[0], gensym("csv-load-status"));
    SETSYMBOL(&stats[1], gensym("lines"));
    SETFLOAT(&stats[2], x->lcount);
    SETSYMBOL(&stats[3], gensym("entries"));
    SETFLOAT(&stats[4], x->lnumload);
    SETSYMBOL(&stats[5], gensym("error"));
    SETFLOAT(&stats[6], x->lnumerror);
    outlet_list(x->x_obj.ob_outlet, &s_list, 7, &stats[0]);
}

/* apuredis */

/* apuredis setup method */
static void setup_apuredis(void)
{
    apuredis_class = class_new(gensym("apuredis"),
        (t_newmethod)redis_new,
        (t_method)redis_free,
        sizeof(t_redis),
        CLASS_DEFAULT,
        A_GIMME, 0);
    
    class_addbang(apuredis_class,apuredis_bang);
    class_addmethod(apuredis_class,
        (t_method)apuredis_stop, gensym("stop"),0);
    class_addmethod(apuredis_class,
        (t_method)apuredis_start, gensym("start"),0);
    class_addmethod(apuredis_class,
        (t_method)redis_command, gensym("command"),
        A_GIMME, 0);
    class_sethelpsymbol(apuredis_class, gensym("apuredis-help"));
}

/* apuredis data yielding callback */ 
static void apuredis_yield(t_redis * x)
{
    if (x->async_num > 0) {
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
                x->async_num--;
                redisReply * reply = (redisReply*)tmpreply;
                redis_parseReply(x, reply);
            }
        } else {
            x->async_num--;
            redisReply * reply = (redisReply*)tmpreply;
            redis_parseReply(x, reply);
        }
    }
    
    if (x->async_prev_num != x->async_num) {
      x->async_prev_num = x->async_num;
      apuredis_q_out(x);
    }
}

/* apuredis outputs queue lenght on second outlet */
static void apuredis_q_out(t_redis * x)
{
    t_atom value;
    SETFLOAT(&value, x->async_num);
    outlet_float(x->q_out, atom_getfloat(&value));
}

/* apuredis scheduled callback */
static void apuredis_run(t_redis *x)
{
    apuredis_yield(x);
    apuredis_schedule(x);
}

/* apuredis (re-)scheduling method */
static void apuredis_schedule(t_redis *x)
{
    if ((!x->async_run) || x->async_num < 1) {
        clock_unset(x->async_clock);
    } else if (x->async_run && x->async_num > 0) {
        clock_delay(x->async_clock, 1);
    }
}

/* apuredis manual yielding method w/bang */
void apuredis_bang(t_redis *x)
{
    apuredis_yield(x);
}

/* apuredis start message method */
void apuredis_start(t_redis *x, t_symbol *s)
{
    (void)s;
    x->async_run = 1;
    apuredis_schedule(x);
}

/* apuredis stop message method */
void apuredis_stop(t_redis *x, t_symbol *s)
{
    (void)s;
    x->async_run = 0;
    apuredis_schedule(x);
}

/* spuredis */

/* spuredis setup method */
static void setup_spuredis(void)
{
    spuredis_class = class_new(gensym("spuredis"),
        (t_newmethod)redis_new,
        (t_method)redis_free,
        sizeof(t_redis),
        CLASS_DEFAULT,
        A_GIMME, 0);
    
    class_addbang(spuredis_class,spuredis_bang);
    class_addmethod(spuredis_class,
        (t_method)spuredis_stop, gensym("stop"),0);
    class_addmethod(spuredis_class,
        (t_method)spuredis_start, gensym("start"),0);
    class_addmethod(spuredis_class,
        (t_method)spuredis_subscribe, gensym("subscribe"),
        A_GIMME, 0);
    class_addmethod(spuredis_class,
        (t_method)spuredis_subscribe, gensym("unsubscribe"),
        A_GIMME, 0);
    class_sethelpsymbol(spuredis_class, gensym("spuredis-help"));
}

/* spuredis scheduled callback */
static void spuredis_run(t_redis *x)
{
    if (x->async_run) {
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
                redisReply * reply = (redisReply*)tmpreply;
                redis_parseReply(x, reply);
            }
        } else {
            redisReply * reply = (redisReply*)tmpreply;
            redis_parseReply(x, reply);
        }
        
        clock_delay(x->async_clock, 100);
    }
}

/* spuredis (re-)scheduling method */
static void spuredis_schedule(t_redis *x)
{
    if (x->async_run && x->async_num < 1) {
        x->async_run = 0;
        clock_unset(x->async_clock);
    } else if ((!x->async_run) && x->async_num > 0) {
        x->async_run = 1;
        clock_delay(x->async_clock, 0);
    }
}

/* spuredis subscriptions management */
static void spuredis_manage(t_redis *x, t_symbol *s, int argc)
{
    if (s == gensym("subscribe")) {
        x->async_num = x->async_num + argc;
    } else {
        x->async_num = x->async_num - argc;
    }
    
    spuredis_schedule(x);
}

/* apuredis alternate start method with bang */
void spuredis_bang(t_redis *x)
{
    spuredis_schedule(x);
}

/* apuredis start message method */
void spuredis_start(t_redis *x, t_symbol *s)
{
    (void)s;
    spuredis_schedule(x);
}

/* apuredis stop message  method */
void spuredis_stop(t_redis *x, t_symbol *s)
{
    (void)s;
    x->async_run = 0;
}

/* apuredis subscribe message  method */
void spuredis_subscribe(t_redis *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc < 1) {
        post("spuredis: subscribe need at least one channel"); return;
    }
    
    int i;
    char ** vector = NULL;
    size_t * lengths = NULL;
    
    if (((vector = malloc((argc+1)*sizeof(char*))) == NULL) || ((lengths = malloc((argc+1)*sizeof(size_t))) == NULL)) {
        post("puredis: can not proceed!!  Memory Error!"); return;
    }
    
    /* setting subscribe or unscribe as redis first command part */
    if (s == gensym("subscribe")) {
        if ((vector[0] = malloc(strlen("subscribe")+1)) == NULL) {
            post("puredis: can not proceed!!  Memory Error!"); return;
        }
        strcpy(vector[0], "subscribe");
        lengths[0] = strlen(vector[0]);
    } else {
        if ((vector[0] = malloc(strlen("unsubscribe")+1)) == NULL) {
            post("puredis: can not proceed!!  Memory Error!"); return;
        }
        strcpy(vector[0], "unsubscribe");
        lengths[0] = strlen(vector[0]);
    }
    
    for (i = 0; i < argc; i++) {
        char cmdpart[256];
        atom_string(argv+i, cmdpart, 256);
        
        if ((vector[i+1] = malloc(strlen(cmdpart)+1)) == NULL) {
            post("puredis: can not proceed!!  Memory Error!"); return;
        }
        
        strcpy(vector[i+1], (char*)cmdpart);
        lengths[i+1] = strlen(vector[i+1]);
    }
    redis_postCommandAsync(x, argc+1, vector, lengths);
    spuredis_manage(x, s, argc);
}

