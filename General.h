#define RT_SIZE		8000 // RT包的长度
#define NRT_SIZE	500 // NRT包的长度
//#define TIMEOUT_INT_RT	0.06 // RT包的timeout时长
//#define TIMEOUT_INT_NRT	0.015 // NRT包的timeout时长
//#define TIMEOUT_INT_RT	0.0014 // RT包的timeout时长
//#define TIMEOUT_INT_NRT	0.00035 // NRT包的timeout时长
#define TIMEOUT_INT_RT	0.016 	// RT包的timeout时长
#define TIMEOUT_INT_NRT	0.001 	// NRT包的timeout时长

//#define DEADLINE_ON // 如果用deadline方法， define它，如果用time_tag方法，comment它

#define RT_REQ_DELAY	0.04 // 用户对RT包提出的QoS delay要求
#define NRT_REQ_DELAY	0.2 // 用户对NRT包提出的QoS delay要求
#define RT_DELAY	0.04 // scheduler对RT包的delay要求，可以与用户的QoS要求不同
//#define NRT_DELAY	0.4 // scheduler对NRT包的delay要求，可以与用户的QoS要求不同
#define NRT_DELAY	1000 // scheduler对NRT包的delay要求，可以与用户的QoS要求不同
//#define RT_TAG_SHIFT	0.01
#define RT_TAG_SHIFT	0

#define RT_C_NUM		8
//#define RT_EXPLAMDA 	8 // RT包 exponential arrival rate lambda value.
#define RT_PKTRATE 	 	64	// RT包 rate.
#define PT_PKTINT		0.015625	// RT包间隔
#define NRT_EXPLAMDA 	32 // NRT包 exponential arrival rate lambda value.

#define PRIORITY_QUEUE

//#define PS_ON_EXPLAMDA 	0.2
//#define PS_OFF_EXPLAMDA 0.2
#define PS_ON_EXPLAMDA 	5 // PU exponential ON period lambda value
#define PS_OFF_EXPLAMDA 0.1 // PU exponential OFF period lambda value

#define RAY_ROW			1 // 从 bandwidth 计算 bitrate 时的 Rayleigh 分布 row 参数
//#define BW_BR_S 		25.3 // Sending power
#define BW_BR_S 		316.2 // Sending power

#define BW_BR_N0	 	0.000001 // Noise level
#define BW_BR_N0_P		0.001 // Noise level

#define CLIENT_MAX_RT_Q_COST	24000 // 40Kb client-side buffer size
#define CLIENT_MAX_NRT_Q_COST	24000 // 40Kb client-side buffer size

#define RETX_LIMIT		4 // timeout counter，如果在primary channel连续出现RETX_LIMIT个timeout，将视为PU on

#define PRIMARY_BW			16560000 // Primary channel total bandwidth
//#define SECONDARY_BW		16000000 // Secondary channel total bandwidth, In total 50 MHz, 50M / 3

#define MAX_UP_UNITS		1 // 一个上传包最多可占用的unit个数
#define MAX_DOWN_UNITS		1 // 一个下传包最多可占用的unit个数

#define UP_BW_UNIT			1380000 // 上传包占用的unit大小
#define DOWN_BW_UNIT		1380000 // 下传包占用的unit大小

//#define EstBW_TO_BR		2.31 // 经Rayleigh分布统计，估计的一个Hz的带宽所支持的传输biterate
#define EstBW_TO_BR		6.08 // 经Rayleigh分布统计，估计的一个Hz的带宽所支持的传输biterate
#define EstBit_TXTIME	(1/EstBW_TO_BR/UP_BW_UNIT) // 如果用一个unit，1bit数据的传输时间


#define TL_INT			0.01 // Primary channel sensing time span
//#define TNL_INT			2.95
#define TNL_INT			0.19 // Primary channel non-sensing time span

#define PRED_ACC		0.95 // primary channel detection accuracy

//---------------------------------------Below are internal macros, don't change------------------------------------------


#define PS_ACTIVE
#define L_ACTIVE

#define C_DATA		0
#define C_NEW 		1
#define ALLOC_BW 	2
#define OPEN_CH		3
#define TX_RESULT	5
#define B_DATA		6
#define B_INTER_DATA	7
#define RT_DATA_SCH		8
#define NRT_DATA_SCH	9
#define PS_SCH		10 // the periodical internal message of primary channel
#define BR_UPDATE	11
#define TL_START	12
#define TL_FIN		13
#define DOWN_TIMEOUT	14
#define UP_TIMEOUT	15

#define PRIMARY_ON	20
#define PRIMARY_OFF	21

#define PRINT_DPKT	30
#define PRINT_PON	31
#define PRINT_POFF	32
#define PRINT_TLON	33
#define PRINT_PD_ON	34
#define PRINT_PD_OFF	35
#define PRINT_CH_DOWN_USE	36
#define PRINT_CH_UP_USE		37


#define C_BASE			7
#define CELL_REMAINDER 	2
#define PS_NUM			2 // number of primary stations
#define CELL_NUM		2 // number of cells in total
#define MAX_C_INCELL 	5000

#define OUT_PATH_PREFIX "result/output_cell"
#define STAT_PATH_PREFIX "result/stat_cell"
#define BW_STAT_PATH_PREFIX "result/bw_cell"
#define STAT_INT	1



//#define C_DEBUG
//#define BS_DEBUG
//#define BW_DEBUG // Print bandwidth allocation
//#define TL_DEBUG
//#define CLIENT_DEBUG // Limit the scope


#define CELL_LOCATION \
	for(int i = 0; i < PS_NUM; i ++) \
		for(int j = 0; j < CELL_NUM; j ++) \
			cellLoc[i][j] = -1; \
	cellLoc[0][0] = 0; \
	cellLoc[1][0] = 1;

#define MAX_SIMTIME 30


#define BR_UPDATE_INT	0.01 // unused
//#define BR_UPDATE_ACTIVE
