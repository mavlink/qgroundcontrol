#ifndef _GOOMTOOLS_H
#define _GOOMTOOLS_H

#define NB_RAND 0x10000

#define RAND_INIT(gd,i) \
        srand (i); \
        if (gd->rand_tab == NULL) \
                gd->rand_tab = g_malloc (NB_RAND * sizeof(gint)) ;\
        gd->rand_pos = 0; \
        while (gd->rand_pos < NB_RAND) \
                gd->rand_tab [gd->rand_pos++] = rand ();

#define RAND(gd) \
        (gd->rand_tab[gd->rand_pos = ((gd->rand_pos + 1) % NB_RAND)])

#define RAND_CLOSE(gd) \
        g_free (gd->rand_tab); \
        gd->rand_tab = NULL;

/*#define iRAND(i) ((guint32)((float)i * RAND()/RAND_MAX)) */
#define iRAND(gd,i) (RAND(gd) % i)
        
#endif
