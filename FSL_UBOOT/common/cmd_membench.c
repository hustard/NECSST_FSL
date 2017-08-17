#include <common.h>
#include <command.h>


unsigned long time;
unsigned long seed;

static void srand_mb( unsigned long s );
static unsigned long rand_mb(void);
static int atoh(char *str);
static int atoi(char *str);

int persist_test (unsigned long* start, unsigned long size, unsigned long val );
int read_test ( unsigned long* start, unsigned long size, int dist, int num_nop );
int write_test ( unsigned long* start, unsigned long size, int dist, int num_nop);
int do_membench( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[] )
{
	unsigned long *start_addr;
	unsigned long size;
	int rw;
	int random;
	unsigned long dist;
	int num_nop;
	if( argc < 3 )
	{
		printf("Error\n");
		return -1;
	}

	start_addr = (unsigned long*)atoh(argv[1]);
	size = (unsigned long)atoh(argv[2]);
	rw = (int)atoi(argv[3]);
	dist = (int)atoi(argv[4]);
	num_nop = (int)atoi(argv[5]);
	//printf("Test Start addr : %p\nSize : %p\nRW : %d\nDist : %p\n",start_addr,size,rw,dist);

	switch(rw)
	{
		case 1: // Read test
			read_test( start_addr, size, dist,num_nop );
			break;
		case 2: // Write test
			write_test( start_addr, size, dist,num_nop );
			break;
		case 3: // Persistency test
			persist_test( start_addr, size, dist );
			break;
		default:
			printf("Input error\n");
			return -1;
	}

	printf("Time : %lu\n",time);
	
}
U_BOOT_CMD(membench,6,1,do_membench,"memory benchmark",\
		"arg1: start address\n" \
		"arg2: size\n" \
		"arg3: read(1), write(2)\n" \
		"arg4: dist ( negative value for random r/w )\n" \
		);

#define ARRAY_SIZE 0x1000000
int persist_test (unsigned long* start, unsigned long size, unsigned long val )
{
	unsigned long *s = start;
	unsigned long cnt = 0;
	unsigned long b_cnt = 0;
	int i;

	for( i=0;i< size/4; i++)
	{
		if( *s != val )
		{
			unsigned long ss = *s;
			unsigned long vv = val;
			cnt++;
			
			while( ss || vv )
			{
				if( (ss&1) != (vv&1) )
					b_cnt++;
				ss = ss>>1;
				vv = vv>>1;
			}
			
			printf("Addr[0x%p] : 0x%p\n",s, *s );
		}
		s++;
	}
	printf("Result: [%d/%d] words have wrong value\n",
			 cnt, size/4 );
	printf("Result: [%d/%d] bits have wrong value\n",
			 b_cnt, size*8 );
}


int read_test ( unsigned long* start, unsigned long size, int dist, int num_nop )
{
	volatile unsigned long tmp;
	register int index = 0;
	unsigned long int i;
	long flags;


	__asm__ __volatile__(
			"mrc p15, 0, %0, c0, c0, 1"::"r"(flags) );
	printf("=== Flags: %p\n",flags );
	time = get_timer(0);

	
	__asm__ __volatile__(
			"mov r1, #0\n\t"
			"LOOP:\n\t"
			"ldr r8, [%0]\n\t"  // Memory access

			"add %0, %2\n\t"
/*
			"mov r2, #0\n\t"	// 
			"1:\n\t"			//
			"add r2, #1\n\t"	//	Dummy Loop
			"cmp r2, %3\n\t"	//
			"bne 1b\n\t"		//
*/
			"add r1, #4\n\t"
			"cmp r1, %1\n\t"
			
			"bne LOOP\n\t"
			:
			:"r"(start), "r"(size), "r"(dist), "r"(num_nop), "r"(time)
			);
		
	/*
	for (i=0 ; i<size ; i+= sizeof(unsigned long) )
	{
		tmp = *( (unsigned long* )( (char* )start + index ) );

		if( dist >= 0 )
			index = ( index + dist )%size;
		else
			index = ( rand_mb()*4 )%size ;
	}
	*/
	time = get_timer(time);
}

int write_test ( unsigned long* start, unsigned long size, int dist,int num_nop )
{
	volatile unsigned long tmp;
	register int index = 0;
	unsigned long int i,j;

	tmp = 0xFFFFFFFF;

	time=get_timer(0);
	__asm__ __volatile__(
			"mov r1, #0\n\t"
			"LOOP2:\n\t"
			"str %5, [%0]\n\t"  // Memory access

			"add %0, %2\n\t"	// Sequential

/*
			"mov r2, #0\n\t"	// 
			"1:\n\t"			//
			"add r2, #1\n\t"	//	Dummy Loop
			"cmp r2, %3\n\t"	//
			"bne 1b\n\t"		//
*/
			"add r1, #4\n\t"	
			"cmp r1, %1\n\t"
			
			"bne LOOP2\n\t"
			:
			:"r"(start), "r"(size), "r"(dist), "r"(num_nop), "r"(time), "r"(tmp)
			);

	/*	for (i=0 ; i<size ; i+= sizeof(unsigned long) )
	{
		*( (unsigned long* )( (char* )start + index ) )=tmp;

		if( dist >= 0 )
			index = ( index + dist ) % size;
		else
			index = ( rand_mb()*4 ) % size ;

		for ( j=0; j< 10 ; j++ )
			asm volatile("nop");
	}*/
	time=get_timer(time);
}

static int atoh(char *str)
{
	int num=0;
	do
	{
		if ( 'a' <= *str && *str <= 'f' )
			*str -= ( 'a'-'A' );
		if ( 'A' <= *str && *str <= 'F' ) 
			num = num*16 + (int)(*str-'A'+10);
		else if ( '0' <= *str && *str <='9' )
			num = num*16 + (int)(*str-'0');
		else
			return -1;
	}while( *(++str) );
	return num;
}
static int atoi(char *str)
{
	int num=0;
	do
	{
		if( *str < '0' || *str > '9' )
			return -1;
		num = num*10 + (int)*str-'0';
	}while( *(++str) );
	return num;
}

static void srand_mb( unsigned long s )
{
	seed = s;
}
static unsigned long rand_mb(void)
{
	unsigned long* seedp = &seed;
	*seedp ^= (*seedp << 13);
	*seedp ^= (*seedp >> 17);
	*seedp ^= (*seedp << 5);

	return *seedp;
}
