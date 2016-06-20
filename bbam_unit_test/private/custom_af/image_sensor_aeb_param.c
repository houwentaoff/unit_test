#include "img_struct.h"

statistics_config_t is_tile_config = {
	1,
	1,

	24,
	16,
	128,
	48,
	160,
	250,
	160,
	250,
	0,
	16383,

	12,
	8,
	0,
	8,
	340,
	512,

	8,
	5,
	256,
	10,
	448,
	816,
	448,
	816,

	0,
	16383,
};

line_t is_50hz_lines[] = {
		{
		{{SHUTTER_1BY8000, ISO_100, 0}}, {{SHUTTER_1BY100, ISO_100,0}}
		},

		{
		{{SHUTTER_1BY100, ISO_100, 0}}, {{SHUTTER_1BY100, ISO_300, 0}}
		},

		{
		{{SHUTTER_1BY50, ISO_100, 0}}, {{SHUTTER_1BY50, ISO_800,0}}
		},

		{
		{{SHUTTER_1BY30, ISO_100, 0}}, {{SHUTTER_1BY30, ISO_6400,0}}
		}
};

line_t is_60hz_lines[] = {
		{
		{{SHUTTER_1BY8000, ISO_100, 0}}, {{SHUTTER_1BY120, ISO_100,0}}
		},

		{
		{{SHUTTER_1BY120, ISO_100, 0}}, {{SHUTTER_1BY120, ISO_300, 0}}
		},

		{
		{{SHUTTER_1BY60, ISO_100, 0}}, {{SHUTTER_1BY60, ISO_800,0}}
		},

		{
		{{SHUTTER_1BY30, ISO_100, 0}}, {{SHUTTER_1BY30, ISO_6400,0}}
		}
};
 
img_awb_param_t is_awb_param = {
	{
		{1495, 1024, 1710},	//AUTOMATIC
		{1082, 1024, 2311},	//INCANDESCENT
		{1245, 1024, 1985},	//D4000
		{1495, 1024, 1710},	//D5000
		{1646, 1024, 1536},	//SUNNY
		{1805, 1024, 1520},	//CLOUDY
		{1495, 1024, 1710},	//FLASH
		{1024, 1024, 1024},	//FLUORESCENT
		{1024, 1024, 1024},	//FLUORESCENT_H
		{1024, 1024, 1024},	//UNDER WATER
		{1024, 1024, 1024},	//CUSTOM
	},
	{
		12,
		{{ 900, 1250, 2000, 2650, -1000, 3000, -1000, 3750, 1000,   900, 1000,1650, 1 },	// 0	INCANDESCENT
		 {1050, 1425, 1675, 2325, -1000, 2800, -1000, 3600, 1000,   400, 1000,1200, 1 },	// 1    D4000
		 {1275, 1675, 1375, 2075, -1000, 2750, -1000, 3600, 1000, -100, 1000,  700, 1 },	// 2	 D5000
		 {1450, 1825, 1350, 1875, -1000, 2750, -1000, 3525, 1000, -450, 1000,  300, 1 },	// 3    SUNNY
		 {1575, 2000, 1325, 1850, -1000, 2850, -1000, 3650, 1000, -650, 1000,  150, 1 },	// 4    CLOUDY
		 {1550, 2000, 1675, 2150, -700,   2900, -1024, 4050, 1600,-1350, 1024, 550, -1 },	// 5    D9000
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 6	D10000
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 7    FLASH
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 8	FLUORESCENT
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 9    FLUORESCENT_2
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 10	FLUORESCENT_3
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 }}//CUSTOM
	},
	{	{ 0 ,6},	//LUT num. AUTOMATIC  INDOOR
		{ 0, 1},	//LUT num. INCANDESCENT
		{ 1, 1},	//LUT num. D4000
		{ 2, 1},	//LUT num. D5000
		{ 2, 5},	//LUT num. SUNNY
		{ 4, 3},	//LUT num. CLOUDY
		{ 7, 1},	//LUT num. FLASH
		{ 8, 1},	//LUT num. FLUORESCENT
		{ 9, 1},	//LUT num. FLUORESCENT_H
		{11, 1},	//LUT num. UNDER WATER
		{11, 1},	//LUT num. CUSTOM
		{ 0, 7},	//LUT num. AUTOMATIC  OUTDOOR
	 }
};
u32 is_ae_agc_dgain[769]={
			1079 ,1086 ,1091 ,1098 ,1103 ,1110 ,1077 ,1082 ,1089 ,1094 ,1101 ,1106 ,1076 ,1082 ,1087 ,1093 ,
			1099 ,1105 ,1111 ,1079 ,1086 ,1091 ,1098 ,1103 ,1110 ,1079 ,1084 ,1089 ,1096 ,1101 ,1108 ,1079 ,
			1082 ,1089 ,1096 ,1101 ,1108 ,1113 ,1081 ,1087 ,1093 ,1098 ,1105 ,1110 ,1079 ,1084 ,1091 ,1096 ,
			1103 ,1108 ,1115 ,1084 ,1089 ,1096 ,1101 ,1108 ,1113 ,1082 ,1087 ,1094 ,1099 ,1106 ,1111 ,1082 ,
			1087 ,1094 ,1099 ,1103 ,1111 ,1116 ,1084 ,1091 ,1096 ,1103 ,1108 ,1115 ,1084 ,1091 ,1096 ,1101 ,
			1108 ,1115 ,1120 ,1089 ,1094 ,1103 ,1106 ,1113 ,1118 ,1086 ,1091 ,1098 ,1103 ,1110 ,1115 ,1084 ,
			1091 ,1096 ,1103 ,1108 ,1115 ,1120 ,1087 ,1094 ,1099 ,1106 ,1111 ,1118 ,1087 ,1094 ,1099 ,1106 ,
			1111 ,1118 ,1123 ,1091 ,1098 ,1103 ,1110 ,1116 ,1123 ,1089 ,1096 ,1101 ,1108 ,1113 ,1120 ,1057 ,
			1064 ,1069 ,1076 ,1081 ,1086 ,1094 ,1062 ,1067 ,1072 ,1079 ,1084 ,1091 ,1060 ,1067 ,1072 ,1079 ,
			1082 ,1089 ,1094 ,1062 ,1069 ,1074 ,1079 ,1086 ,1091 ,1060 ,1067 ,1072 ,1077 ,1084 ,1089 ,1058 ,
			1064 ,1070 ,1076 ,1081 ,1087 ,1096 ,1062 ,1069 ,1074 ,1079 ,1084 ,1091 ,1060 ,1065 ,1072 ,1077 ,
			1082 ,1089 ,1094 ,1065 ,1070 ,1076 ,1082 ,1087 ,1094 ,1062 ,1067 ,1074 ,1079 ,1086 ,1091 ,1060 ,
			1067 ,1072 ,1079 ,1084 ,1087 ,1094 ,1064 ,1069 ,1076 ,1081 ,1087 ,1093 ,1062 ,1069 ,1074 ,1081 ,
			1086 ,1093 ,1098 ,1065 ,1072 ,1079 ,1084 ,1091 ,1096 ,1064 ,1069 ,1076 ,1081 ,1086 ,1093 ,1062 ,
			1067 ,1074 ,1081 ,1086 ,1091 ,1096 ,1065 ,1072 ,1077 ,1082 ,1089 ,1094 ,1064 ,1069 ,1074 ,1081 ,
			1086 ,1093 ,1099 ,1069 ,1074 ,1079 ,1086 ,1091 ,1098 ,1065 ,1070 ,1077 ,1082 ,1089 ,1096 ,1040,
			1046,1051,1057,1063,1069,1074,1044,1049,1055,1061,1067,1072,1043,1046,1054,1058,1066,1072,
			1078,1044,1050,1056,1064,1067,1073,1043,1049,1055,1060,1066,1072,1041,1047,1053,1061,1067,
			1070,1076,1044,1050,1058,1061,1067,1073,1042,1048,1053,1059,1065,1071,1076,1047,1053,1058,
			1064,1070,1076,1043,1049,1055,1060,1066,1072,1043,1049,1052,1058,1064,1070,1077,1045,1051,
			1057,1063,1068,1072,1044,1051,1057,1061,1068,1072,1080,1047,1053,1058,1066,1070,1076,1044,
			1050,1055,1061,1067,1073,1044,1048,1055,1061,1065,1071,1077,1046,1051,1057,1063,1069,1074,
			1043,1050,1056,1062,1066,1073,1079,1049,1053,1059,1066,1072,1078,1046,1050,1057,1063,1069,
			1075,1022,1028,1036,1040,1045,1050,1057,1025,1031,1036,1042,1048,1053,1023,1028,1034,1039,
			1046,1051,1058,1024,1030,1037,1041,1048,1052,1023,1029,1034,1040,1045,1051,1021,1026,1032,
			1037,1042,1049,1054,1024,1028,1035,1041,1045,1051,1021,1027,1032,1036,1043,1049,1054,1024,
			1029,1035,1041,1045,1051,1021,1024,1027,1035,1040,1046,1015,1021,1027,1033,1038,1044,1050,
			1018,1024,1029,1035,1041,1046,1018,1023,1029,1035,1041,1045,1051,1021,1026,1032,1038,1043,
			1049,1017,1022,1029,1034,1040,1047,1015,1021,1026,1032,1037,1043,1050,1020,1026,1031,1037,
			1043,1048,1017,1023,1028,1035,1040,1045,1052,1022,1027,1032,1039,1044,1049,1019,1024,1030,
			1036,1041,1046,1016,1022,1029,1033,1038,1044,1050,1021,1027,1032,1038,1044,1049,1019,1025,
			1031,1036,1042,1047,1053,1023,1029,1034,1039,1046,1051,1021,1026,1031,1037,1044,1049,1019,
			1024,1029,1036,1042,1047,1053,1021,1026,1032,1038,1043,1049,1019,1025,1030,1036,1041,1046,
			1053,1021,1027,1033,1038,1044,1050,1021,1026,1032,1037,1043,1048,1017,1023,1028,1033,1040,
			1045,1050,1022,1027,1032,1038,1044,1050,1019,1025,1031,1036,1042,1047,1053,1024,1030,1036,
			1041,1048,1053,1020,1026,1031,1037,1042,1048,1020,1026,1031,1036,1042,1048,1054,1022,1027,
			1033,1038,1044,1050,1020,1026,1032,1037,1043,1048,1054,1026,1031,1037,1043,1049,1054,1021,
			1027,1032,1038,1044,1049,1018,1022,1026,1033,1038,1042,1047,1016,1023,1031,1038,1045,1051,
			1022,1027,1033,1038,1044,1050,1055,1025,1031,1037,1042,1048,1054,1024,1029,1036,1040,1047,
			1052,1020,1025,1031,1037,1042,1048,1053,1024,1030,1035,1041,1046,1053,987,992,998,1003,
			1009,1015,1020,989,994,999,1003,1007,1012,986,991,997,1002,1008,1014,987,993,999,1005,1010,
			1016,1021,991,996,1001,1007,1012,1018,987,993,998,1004,1009,1014,1020,995,1001,1006,1012,
			1017,1023,988,994,999,1005,1010,1016,988,993,999,1004,1010,1016,1021,993,998,1003,1009,1015,
			1019,987,992,997,1003,1008,1015,1021,990,995,1001,1006,1012,1017,991,996,1002,1007,1013,1018,1024,
	};
u32 is_ae_sht_dgain[1036] =
{
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
};
u8 is_dlight[2] = {128,4};
