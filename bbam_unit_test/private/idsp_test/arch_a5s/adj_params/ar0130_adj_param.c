/**
 * packages/imgproc/packages/mt9m033_adj_param.c
 *
 *
 * Details  : internal idsp tuning parmateters
 *
 * History:
 *	2010/02/01 [Yihe Yao] - created file
 *
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */


#include "img_struct.h"


chroma_scale_filter_t ar0130_chroma_scale = {
					1,
					{
					256,299,342,385,428,471,514,557,600,643,686,729,772,815,858,901,
					936,970,990,1012,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
					1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
					1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
					1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
					1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
					1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
					1024,1012,996,956,916,856,796,736,676,616,556,496,436,376,316,256,
					},
					};

rgb_to_yuv_t ar0130_rgb2yuv[4] = 
	{
		{//TV
			{187,  629,  63,
			-103, -346, 450,
			450, -409, -41,},
			16,  128, 128
		},
		{//PC
			{306,  601,  117,
			-173, -339, 512,
			512, -429, -83,},
			0,  128, 128
		},
		{//STILL
			{306,  601,  117,
			-173, -339, 512,
			512, -429, -83,},
			0,  128, 128
		},
		{//CUSTOM
			{187,  629,  63,
			-103, -346, 450,
			450, -409, -41,},
			16,  128, 128
		},
	};

adj_param_t ar0130_adj_param = {
	{20,
	//ev_index
	// AWB			            AE     // for Ambarella AE AWB algo
	// low       d50       high         target
	{{{ 128,128,  128,128,  128,128,     122}}, // [0]: outdoor
	 {{ 128,128,  128,128,  128,128,     122 }}, // [1]:6db
	 {{ 128,128,  128,128,  128,128,     122 }}, // [2]:12db
	 {{ 128,128,  128,128,  128,128,     122 }}, // [3]:18db
	 {{ 128,128,  128,128,  128,128,     110 }}, // [4]:24db
	 {{ 128,128,  128,128,  128,128,     90 }}, // [5]:30db
	 {{ 128,128,  128,128,  128,128,     90 }}, // [6]:36db
	 {{ 128,128,  128,128,  128,128,     90 }}, // [7]:42db
	 {{ 128,128,  128,128,  128,128,     90 }}, // [8]:
	 {{ 128,128,  128,128,  128,128,     90 }}, // [9]:
	 {{ 128,128,  128,128,  128,128,     90 }}, // [10]:
	 {{ 128,128,  128,128,  128,128,     90 }}, // [11]:
	 {{ 128,128,  128,128,  128,128,     90 }}, // [12]:
	 {{ 128,128,  128,128,  128,128,     90 }}, // [13]:
	 {{ 128,128,  128,128,  128,128,     90 }}, // [14]:
	 {{ 128,128,  128,128,  128,128,     90 }}, // [15]:
	 {{ 128,128,  128,128,  128,128,     90 }}, // [16]:
	 {{ 128,128,  128,128,  128,128,     90 }}, // [17]:
	 {{ 128,128,  128,128,  128,128,     90 }}, // [18]:
	 {{ 128,128,  128,128,  128,128,     90 }}}},// [19]:

	{20,
	//ev_index
	//  Y   UV  color  Gamma l_exposure chr_scale aliasing
	 {{  0,   0,    1,     1,    1,         1,         0}},  // ev_color enable
	 {{{16,  64,  128,   128,  128,       60,         0}},  // [0]: outdoor
	  {{16,  64,  128,   128,  128,        56,         0}},  // [1]:6db
	  {{16,  64,  128,   128,  128,        52,         0}},  // [2]:12db
	  {{16,  64,  128,   128,  128,        48,         0}},  // [3]:18db
	  {{16,  64,  128,   128,  128,        48,         0}},  // [4]:24db
	  {{16,  64,  128,   128,  128,        48,         0}},  // [5]:30db
	  {{16,  64,  128,   128,  128,        48,         0}},  // [6]:36db
	  {{16,  64,  128,   128,  128,        48,         0}},  // [7]:42db
	  {{16,  64,  128,   128,  128,        48,         0}},  // [8]:
	  {{16,  64,  128,   128,  128,        48,         0}},  // [9]:
	  {{16,  64,  128,   128,  112,        48,         0}},  // [10]:
	  {{16,  64,  128,   128,  100,        48,         0}},  // [12]:
	  {{16,  64,   80,   128,   80,        48,         0}},  // [13]:
	  {{16,  64,   50,   128,   60,        48,         0}},  // [14]:
	  {{16,  64,   30,   128,    0,        48,         0}},  // [15]:
	  {{16,  64,   30,   128,    0,        48,         0}},  // [16]:
	  {{16,  64,   30,   128,    0,        48,         0}},  // [17]:
	  {{16,  64,   30,   128,    0,        48,         0}},  // [18]:
	  {{16,  64,   30,   128,    0,        48,         0}}}},// [19]:

	{10,

	{0,	// color_type

	 1,			// color_control
	 3,			// color_table_count
	{{975, 2500, 0, {0},{ 1644,  -475, -145,	//2800K
			      -554,  1627, -49,
			      130,  -1337, 2231}},
	 {1400, 1650, 0, {0},{ 1779, -645,  -110,	//5000K
			      -375,  1536, -137,
			       77,  -710, 1657}},
	 {1600, 1475, 0, {0},{ 1856, -725, -107,	//7500K
			      -330,  1506, -152,
			      55,  -633, 1602}}},
	{512,640},	// cc_interpo_ev
	},


	//black_level_control
	1, //black_level_control enable
	//        Low                 High
	//    R     G    B        R     G     B
	{{{ -675, -675, -675,   -675, -675, -675 }}, // [0]: 0db
	 {{ -675, -675, -675,   -675, -675, -675 }}, // [1]: 6db
	 {{ -675, -675, -675,   -675, -675, -675 }}, // [2]: 12db
	 {{ -675, -675, -675,   -675, -675, -675 }}, // [3]: 18db
	 {{ -675, -675, -675,   -675, -675, -675 }}, // [4]: 24db
	 {{ -675, -675, -675,   -675, -675, -675 }}, // [5]: 30db
	 {{ -675, -675, -675,   -675, -675, -675 }}, // [6]: 36db
	 {{ -675, -675, -675,   -675, -675, -675 }}, // [7]: 42db
	 {{ -675, -675, -675,   -675, -675, -675 }}, // [8]: 48db
	 {{ -675, -675, -675,   -675, -675, -675 }}},// [9]: 54db

	//Bad Pixel           Low         High color temperture
	//type hBP dBP        cb   cr     cb   cr  chroma_median
	{{   1,                1                    }}, // enable
	{{{  2,  0,  0,        0, 0,   0, 0 }}, // [0]: 0db
	 {{  2,  0,  0,        0, 0,   0, 0 }}, // [1]: 6db
	 {{ 2,  3,  3,        128, 128,   128, 128}}, // [2]: 12db
	 {{ 2,  6,  6,        128, 128,   128, 128}}, // [3]: 18db
	 {{ 3,  8,  8,        255, 255,   255, 255}}, // [4]: 24db
	 {{ 3, 10, 10,        255, 255,   255, 255 }}, // [5]: 30db
	 {{ 3, 10,10,        255, 255,   255, 255 }}, // [6]: 36db
	 {{  3, 10, 10,        255, 255,   255, 255 }}, // [7]: 42db
	 {{  3, 10, 10,        255, 255,   255, 255 }}, // [8]: 48db
	 {{  3, 10, 10,        255, 255,   255, 255}}}, // [9]: 54db

	//cfa_filter
	1, //cfa_filter_enable

	//Low color temperture
	// dir                         non_dir
	//center_weight threshold      center_weight threshold
	//  r   g   b    r    g    b    r   g   b    r    g    b close  grad_thresh
	{{{  8,  12,  8,   0,   0,   0,   12,18,12,   100, 100, 100, 150,   341 }}, // [0]: 0db
	 {{  8,  12,  8,   0,   0,   0,   10,15,10,   300, 300, 300, 300,   341 }}, // [1]: 6db
	 {{  8,  12,  8,  100,   100,100,   6,9,6,   500, 500, 500, 500,   341 }}, // [2]: 12db
	 {{  6,  9,  6,  200,   200,200,   4,6,4,   600, 600, 600, 600,   341 }}, // [3]: 18db
	 {{  6,  9,  6,  800,   800,800,   2,3,2,   800, 800,800, 800,   341 }}, // [4]: 24db
	 {{  6,  9,  6,  1000,   1000,1000,   2,3,2,   1000, 1000,1000, 1000,   341 }}, // [5]: 30db
	 {{  4,  6,  4,  2200,   2200,2200,   2,3,2,   2400, 2400,2400, 2400,   341 }}, // [6]: 36db
	 {{ 2, 3,  2, 4400, 4400,4400,   2, 3,  2, 5500,5500,5500,5500,341}}, // [7]: 42db
	 {{ 2, 3,  2, 4400, 4400,4400,   2, 3,  2, 5500,5500,5500,5500,341}}, // [8]: 48db
	 {{ 2, 3,  2, 4400, 4400,4400,   2, 3,  2, 5500,5500,5500,5500,341}}}, // [9]: 54db

	//High color temperture
	// dir                         non_dir
	//center_weight threshold      center_weight threshold
	//  r   g   b    r    g    b    r   g   b    r    g    b close  grad_thresh
	{{{  8,  12,  8,   0,   0,   0,   12,18,12,   100, 100, 100, 150,   341 }}, // [0]: 0db
	 {{  8,  12,  8,   0,   0,   0,   10,15,10,   300, 300, 300, 300,   341 }}, // [1]: 6db
	 {{  8,  12,  8,  100,   100,100,   6,9,6,   500, 500, 500, 500,   341 }}, // [2]: 12db
	 {{  6,  9,  6,  200,   200,200,   4,6,4,   600, 600,600, 600,   341 }}, // [3]: 18db
	 {{  6,  9,  6,  800,   800,800,   2,3,2,   800, 800,800, 800,   341 }}, // [4]: 24db
	 {{  6,  9,  6,  1000,   1000,1000,   2,3,2,   1000, 1000,1000, 1000,   341 }}, // [5]: 30db
	 {{  4,  6,  4,  2200,   2200,2200,   2,3,2,   2400, 2400,2400, 2400,   341 }}, // [6]: 36db
	 {{ 2, 3,  2, 4400, 4400,4400,   2, 3,  2, 5500,5500,5500,5500,341}}, // [7]: 42db
	 {{ 2, 3,  2, 4400, 4400,4400,   2, 3,  2, 5500,5500,5500,5500,341}}, // [8]: 48db
	 {{ 2, 3,  2, 4400, 4400,4400,   2, 3,  2, 5500,5500,5500,5500,341}}}, // [9]: 54db

	/* ratio_255_gamma_table */
        {
                {/* red */
                   0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,  64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
                 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
                 257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317, 321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
                 385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445, 449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
                 514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574, 578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
                 642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702, 706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
                 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830, 834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
                 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959, 963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
		{/* green */
                   0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,  64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
                 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
                 257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317, 321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
                 385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445, 449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
                 514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574, 578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
                 642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702, 706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
                 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830, 834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
                 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959, 963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
		{/* blue */
                   0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,  64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
                 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
                 257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317, 321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
                 385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445, 449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
                 514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574, 578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
                 642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702, 706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
                 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830, 834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
                 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959, 963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023}
	},

        /* ratio_0_gamma_table */
        {
            {/* red */
                   0,   1,   1,   2,   3,   4,   5,   7,   8,   9,  10,  12,  13,  15,  16,  18, 20,  22,  23,  25,  27,  29,  31,  33,  35,  37,  39,  41,  43,  46,  48,  51,
                  53,  55,  58,  60,  63,  65,  68,  70,  73,  76,  78,  81,  83,  86,  89,  92, 95,  97, 100, 103, 106, 109, 112, 114, 117, 120, 124, 129, 133, 137, 141, 146,
		 150, 154, 158, 163, 167, 171, 175, 180, 184, 189, 194, 199, 204, 210, 215, 220,225, 230, 235, 240, 245, 250, 255, 261, 266, 271, 276, 281, 286, 292, 297, 303,
		 308, 314, 320, 325, 331, 336, 342, 347, 353, 359, 364, 370, 375, 381, 387, 392,398, 403, 409, 414, 420, 425, 430, 436, 441, 446, 451, 457, 462, 467, 472, 478,
		 483, 488, 493, 498, 504, 509, 514, 519, 525, 530, 535, 540, 546, 551, 556, 561,565, 570, 575, 579, 584, 589, 593, 598, 603, 607, 612, 617, 621, 626, 631, 635,
		 640, 645, 649, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
		 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,834, 838, 842, 847, 851, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
		 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
		{/* green */
                   0,   1,   1,   2,   3,   4,   5,   7,   8,   9,  10,  12,  13,  15,  16,  18, 20,  22,  23,  25,  27,  29,  31,  33,  35,  37,  39,  41,  43,  46,  48,  51,
                  53,  55,  58,  60,  63,  65,  68,  70,  73,  76,  78,  81,  83,  86,  89,  92, 95,  97, 100, 103, 106, 109, 112, 114, 117, 120, 124, 129, 133, 137, 141, 146,
		 150, 154, 158, 163, 167, 171, 175, 180, 184, 189, 194, 199, 204, 210, 215, 220,225, 230, 235, 240, 245, 250, 255, 261, 266, 271, 276, 281, 286, 292, 297, 303,
		 308, 314, 320, 325, 331, 336, 342, 347, 353, 359, 364, 370, 375, 381, 387, 392,398, 403, 409, 414, 420, 425, 430, 436, 441, 446, 451, 457, 462, 467, 472, 478,
		 483, 488, 493, 498, 504, 509, 514, 519, 525, 530, 535, 540, 546, 551, 556, 561,565, 570, 575, 579, 584, 589, 593, 598, 603, 607, 612, 617, 621, 626, 631, 635,
		 640, 645, 649, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
		 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,834, 838, 842, 847, 851, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
		 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
		{/* blue */
                   0,   1,   1,   2,   3,   4,   5,   7,   8,   9,  10,  12,  13,  15,  16,  18, 20,  22,  23,  25,  27,  29,  31,  33,  35,  37,  39,  41,  43,  46,  48,  51,
                  53,  55,  58,  60,  63,  65,  68,  70,  73,  76,  78,  81,  83,  86,  89,  92, 95,  97, 100, 103, 106, 109, 112, 114, 117, 120, 124, 129, 133, 137, 141, 146,
		 150, 154, 158, 163, 167, 171, 175, 180, 184, 189, 194, 199, 204, 210, 215, 220,225, 230, 235, 240, 245, 250, 255, 261, 266, 271, 276, 281, 286, 292, 297, 303,
		 308, 314, 320, 325, 331, 336, 342, 347, 353, 359, 364, 370, 375, 381, 387, 392,398, 403, 409, 414, 420, 425, 430, 436, 441, 446, 451, 457, 462, 467, 472, 478,
		 483, 488, 493, 498, 504, 509, 514, 519, 525, 530, 535, 540, 546, 551, 556, 561,565, 570, 575, 579, 584, 589, 593, 598, 603, 607, 612, 617, 621, 626, 631, 635,
		 640, 645, 649, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
		 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,834, 838, 842, 847, 851, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
		 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023}
	},
	/* ratio_255_local_exposure */
        {1024,4095,3213,2788,2521,2331,2187,2072,
	1978,1898,1829,1769,1716,1669,1626,1587,
	1552,1519,1489,1461,1435,1411,1388,1367,
	1346,1327,1309,1292,1276,1260,1245,1231,
	1217,1207,1199,1190,1181,1173,1163,1154,
	1146,1137,1129,1122,1113,1106,1099,1093,
	1086,1080,1075,1069,1064,1060,1055,1051,
	1047,1043,1040,1037,1034,1031,1030,1027,
	1025,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024},
        /* ratio_0_local_exposure */
        {1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024},
	},
	{10,
	//video_mctf
	1, //video_mctf_enable
	// alpha   thr1   thr2     y    u    v max_change
	{{{120,     2,     4,   50, 100, 100}},  // [0]: 0db
	 {{ 112,     2,     4,   50, 100, 100}},  // [1]: 6db
	 {{ 96,     2,     4,   50, 100, 100}},  // [2]: 12db
	 {{ 96,     4,     8,   50, 100, 100}},  // [3]: 18db
	 {{ 64,     5,     10,   50, 100, 100}},  // [4]: 24db
	 {{ 64,     8,     16,   50, 100, 100}},  // [5]: 30db
	 {{   64,     20,    40,   50, 100, 100}},  // [6]: 36db
	 {{   64,     20,    40,   50, 100, 100}},  // [7]: 42db
	 {{   64,     20,    40,   50, 100, 100}},  // [8]: 48db
	 {{   64,     20,    40,   50, 100, 100}}}},// [9]: 54db

	 {10,

	//spatial_filter
	1, //spatial_enable
	// isotropic directional edge_thr
	{{{   50,          0,        35 }}, // [0]: 0db
	 {{   60,          0,        50 }}, // [1]: 6db
	 {{    80,         10,       70 }}, // [2]: 12db
	 {{    90,         10,       70 }}, // [3]: 18db
	 {{    100,         15,       80 }}, // [4]: 24db
	 {{    120,         20,       90 }}, // [5]: 30db
	 {{    160,         30,       110 }}, // [6]: 36db
	 {{  190,         40,       150 }}, // [7]: 42db
	 {{  190,         40,       150 }}, // [8]: 48db
	 {{  190,         40,       150 }}}, // [9]: 54db

	//sharpening_filter
	1, //sharpening_enable

	//sharpening_level_minimum
	//  low  low_delta  low_str  mid_str  high  high_delta  high_str
	{{{  40,      5,       0,          0, 180,       4,    0   }}, // [0]: 0db
	 {{  40,      5,       0,          0, 180,       4,      0   }}, // [1]: 6db
	 {{  40,      5,       0,          0, 180,       4,      0   }}, // [2]: 12 db
	 {{  40,      5,       0,          0, 180,       4,      0   }}, // [3]: 18db
	 {{  40,      5,       0,          0, 180,       4,      0   }}, // [4]: 24db
	 {{  40,      5,       0,          0, 180,       4,      0   }}, // [5]: 30db
	 {{  40,      5,       0,          0, 180,       4,      0   }}, // [6]: 36db
	 {{  40,      5,       0,          0, 180,       4,      0   }}, // [7]: 42db
	 {{  40,      5,       0,          0, 180,       4,      0   }}, // [8]: 48db
	 {{  40,      5,       0,          0, 180,       4,      0   }}}, // [9]: 54db

	//sharpening_level_overall
	//  low  low_delta  low_str  mid_str  high  high_delta  high_str
	{{{   40,      5,    128,    160,   180,       4,     96}}, // [0]: 0db
	 {{   40,      5,    96,    128,   180,       4,     64}}, // [1]: 6db
	 {{   40,      5,    64,    112,   180,       4,     48}}, // [2]: 12db
	 {{   40,      5,    48,    72,   180,       4,     36}}, // [3]: 18db
	 {{   40,      5,    24,    48,   180,       4,     24}}, // [4]: 24db
	 {{   40,      5,    8,    32,   180,       4,     8}}, // [5]: 30db
	 {{   40,      5,    8,    24,   180,       4,     8}}, // [6]: 36db
	 {{   40,      5,    8,    24,   180,       4,     8}}, // [7]: 42db
	 {{   40,      5,    8,    24,   180,       4,     8}}, // [8]: 48db
	 {{   40,      5,    8,    24,   180,       4,     8}}}, // [9]: 54db

	//high_freq   retain    max_change      fir_filter
	//noise       level      up   down      strength    coeff
	{{{    8,       0,     40, 30,     192,         0,   0,   0,   0,   0,   0 }},  // [0]: 0db
	 {{    12,       0,     30, 20,     192,         0,   0,   0,   0,   0,   0 }},  // [1]: 6db
	 {{    12,       0,     12, 10,      192,         0,   0,   0,   0,   0,   0 }},  // [2]: 12db
	 {{    12,       0,     10, 10,      192,         0,   0,   0,   0,   0,   0 }},  // [3]: 18db
	 {{    16,       0,     8, 8,      192,         0,   0,   0,   0,   0,   0 }},  // [4]: 24db
	 {{    16,       0,     8, 8,      192,         0,   0,   0,   0,   0,   0 }},  // [5]: 30db
	 {{16,       0,     4, 6,       192,         0,   0,   0,   0,   0,   0 }},  // [6]: 36db
	 {{16,       0,     4, 4,       192,         0,   0,   0,   0,   0,   0 }},  // [7]: 42db
	 {{16,       0,     4, 4,       192,         0,   0,   0,   0,   0,   0 }},  // [8]: 48db
	 {{16,       0,     4, 4,       192,         0,   0,   0,   0,   0,   0 }}}, // [9]: 54db

	//coring
        {{{ // [0]: 0db
           31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
           31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
           31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
	   31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 16, 16, 16, 16, 8, 4,
	    4, 8,16, 16, 16, 16, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
           31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
           31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
           31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31}},
         {{ // [1]: 6db
          31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
           31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
           31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
	   31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 16, 16, 16, 16, 8, 4,
	    4, 8,16, 16, 16, 16, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
           31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
           31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
           31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31}},
         {{ // [4]: 12db
           24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
           24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
           24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	   24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 16, 16, 16, 16, 8, 4,
	    4, 8, 16, 16, 16, 16, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
          24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
           24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
           24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,}},
         {{ // [4]: 18db
           24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
           24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
           24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	   24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 16, 16, 16, 16, 8, 4,
	    4, 8, 16, 16, 16, 16, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
          24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
           24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
           24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,}},
         {{ // [4]: 24db
           18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
           18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
           18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	   18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 8, 4,
	    4, 8, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
           18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
           18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
           18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18}},
         {{  // [5]: 30db
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,6, 4,  4,
	    4,  4, 6, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12}},
         {{  // [5]: 36db
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,6, 4,  4,
	    4,  4, 6, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12}},
         {{  // [5]: 42db
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,6, 4,  4,
	    4,  4, 6, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12}},
         {{  // [5]: 48db
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,6, 4,  4,
	    4,  4, 6, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12}},
	  {{  // [5]: 54db
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,6, 4,  4,
	    4,  4, 6, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	   12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12}},
	}
	},
};

local_exposure_t ar0130_manual_LE[3] = {
{	1, 0, 16, 16, 16, 2,// 2X
	{2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
	2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
	2047, 2043, 2037, 2029, 2020, 2009, 1997, 1985, 1972, 1958, 1943, 1928, 1913, 1898, 1883, 1867,
	1851, 1836, 1820, 1804, 1788, 1773, 1757, 1742, 1727, 1712, 1697, 1682, 1667, 1653, 1638, 1624,
	1610, 1597, 1583, 1570, 1556, 1543, 1530, 1518, 1505, 1493, 1480, 1468, 1456, 1445, 1433, 1422,
	1411, 1400, 1389, 1378, 1367, 1357, 1347, 1337, 1326, 1317, 1307, 1297, 1288, 1279, 1269, 1260,
	1251, 1242, 1234, 1225, 1217, 1208, 1200, 1192, 1184, 1176, 1168, 1160, 1153, 1145, 1138, 1130,
	1123, 1116, 1109, 1102, 1095, 1088, 1081, 1075, 1068, 1062, 1055, 1049, 1043, 1037, 1031, 1024,
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,}}
,
{	1, 0, 16, 16, 16, 2, // 3X
	{3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072,
	3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072,
	3070, 3063, 3053, 3039, 3023, 3005, 2985, 2964, 2942, 2918, 2894, 2870, 2844, 2819, 2793, 2767,
	2741, 2716, 2690, 2664, 2638, 2613, 2588, 2563, 2538, 2513, 2489, 2465, 2441, 2418, 2395, 2372,
	2350, 2328, 2306, 2285, 2263, 2243, 2222, 2202, 2182, 2162, 2143, 2124, 2105, 2087, 2069, 2051,
	2033, 2016, 1999, 1982, 1965, 1949, 1933, 1917, 1901, 1886, 1871, 1856, 1841, 1827, 1813, 1799,
	1785, 1771, 1758, 1744, 1731, 1718, 1706, 1693, 1681, 1669, 1657, 1645, 1633, 1622, 1610, 1599,
	1588, 1577, 1566, 1556, 1545, 1535, 1525, 1515, 1505, 1495, 1485, 1476, 1466, 1457, 1448, 1439,
	1430, 1421, 1412, 1403, 1395, 1386, 1378, 1370, 1361, 1353, 1345, 1337, 1330, 1322, 1314, 1307,
	1299, 1292, 1285, 1278, 1270, 1263, 1256, 1249, 1243, 1236, 1229, 1223, 1216, 1210, 1203, 1197,
	1191, 1185, 1178, 1172, 1166, 1160, 1155, 1149, 1143, 1137, 1132, 1126, 1120, 1115, 1109, 1104,
	1099, 1094, 1088, 1083, 1078, 1073, 1068, 1063, 1058, 1053, 1048, 1043, 1039, 1034, 1024, 1024,
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,}}
,
{	1, 0, 16, 16, 16, 2, // 4X
	{4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
	4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
	4092, 4083, 4068, 4049, 4027, 4002, 3974, 3945, 3914, 3881, 3848, 3814, 3779, 3744, 3709, 3673,
	3637, 3602, 3566, 3531, 3496, 3461, 3426, 3392, 3358, 3324, 3291, 3259, 3226, 3194, 3163, 3132,
	3102, 3071, 3042, 3013, 2984, 2956, 2928, 2900, 2873, 2847, 2820, 2795, 2769, 2744, 2720, 2696,
	2672, 2648, 2625, 2603, 2580, 2558, 2537, 2515, 2494, 2473, 2453, 2433, 2413, 2394, 2375, 2356,
	2337, 2319, 2301, 2283, 2266, 2248, 2231, 2214, 2198, 2182, 2166, 2150, 2134, 2119, 2103, 2088,
	2074, 2059, 2045, 2030, 2016, 2003, 1989, 1976, 1962, 1949, 1936, 1923, 1911, 1898, 1886, 1874,
	1862, 1850, 1838, 1827, 1816, 1804, 1793, 1782, 1771, 1761, 1750, 1739, 1729, 1719, 1709, 1699,
	1689, 1679, 1669, 1660, 1650, 1641, 1632, 1623, 1614, 1605, 1596, 1587, 1578, 1570, 1561, 1553,
	1545, 1537, 1529, 1520, 1513, 1505, 1497, 1489, 1482, 1474, 1467, 1459, 1452, 1445, 1437, 1430,
	1423, 1416, 1409, 1403, 1396, 1389, 1382, 1376, 1369, 1363, 1356, 1350, 1344, 1338, 1331, 1325,
	1319, 1313, 1307, 1301, 1296, 1290, 1284, 1278, 1273, 1267, 1262, 1256, 1251, 1245, 1240, 1235,
	1229, 1224, 1219, 1214, 1209, 1204, 1199, 1194, 1189, 1184, 1179, 1174, 1170, 1165, 1160, 1156,
	1151, 1146, 1142, 1137, 1133, 1128, 1124, 1120, 1115, 1111, 1107, 1103, 1098, 1094, 1090, 1086,
	1082, 1078, 1074, 1070, 1066, 1062, 1058, 1054, 1050, 1047, 1043, 1039, 1035, 1032, 1028, 1024,}}
};

