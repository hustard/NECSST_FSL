/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <exports.h>
#define NUM_OP 1000
struct node
{
	int data;
	struct node* next;
};

struct node* root;
int hello_world (int argc, char * const argv[])
{
	int i;
	struct node* cur;

	root = malloc(sizeof(struct node) );
	cur = root;
	/* Print the ABI version */
	app_startup(argv);
	
	/* code injection */


	printf ("Example expects ABI version %d\n", XF_VERSION);
	printf ("Actual U-Boot ABI version %d\n", (int)get_version());

	printf ("Hello World\n");

	printf ("argc = %d\n", argc);

	for (i=0; i<=argc; ++i) {
		printf ("argv[%d] = \"%s\"\n",
			i,
			argv[i] ? argv[i] : "<NULL>");
	}

	for(i=0;i<NUM_OP;i++)
	{
		struct node* new = malloc( sizeof(struct node) );
		printf("%p\n",new);
		new->data = i;
		free(cur);
		cur=new;
	}





	printf ("Hit any key to exit ... ");
	while (!tstc())
		;


	/* consume input */
	(void) getc();

	printf ("\n\n");
	return (0);
}
