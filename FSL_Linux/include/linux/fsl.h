#include <linux/gfp.h>

int fsl_enter(void);
/*
struct fsl_ops {
	    int (*valid)(suspend_state_t state);
		    int (*begin)(suspend_state_t state);
			    int (*prepare)(void);
				    int (*prepare_late)(void);
					    int (*enter)(suspend_state_t state);
						    void (*wake)(void);
							    void (*finish)(void);
								    bool (*suspend_again)(void);
									    void (*end)(void);
										    void (*recover)(void);
};
*/
#define NUM_DEV_TRACE 1000


/* Switch for I/O tracing mode... */
extern int device_io_tracing;

/* And its collector */
extern int device_io_trace_cnt;
extern unsigned long dev_trace_addr[NUM_DEV_TRACE];
extern unsigned long dev_trace_value[NUM_DEV_TRACE];
extern short dev_trace_write_len[NUM_DEV_TRACE];
extern struct page* fsl_tmp_page;
