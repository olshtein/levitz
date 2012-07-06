#include "tebahpla.h"
// ASCII to 7-bits
char tebahpla[] = {
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
32,
33,
34,
35,
2,
37,
38,
39,
40,
41,
42,
43,
44,
45,
46,
47,
48,
49,
50,
51,
52,
53,
54,
55,
56,
57,
58,
59,
60,
61,
62,
63,
0,
65,
66,
67,
68,
69,
70,
71,
72,
73,
74,
75,
76,
77,
78,
79,
80,
81,
82,
83,
84,
85,
86,
87,
88,
89,
90,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
17,
NULL_CHAR,
97,
98,
99,
100,
101,
102,
103,
104,
105,
106,
107,
108,
109,
110,
111,
112,
113,
114,
115,
116,
117,
118,
119,
120,
121,
122,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
NULL_CHAR,
};

/*
 * return CHARACTER of selected ch
 */
CHARACTER getCHAR(char ch,bool selected){
	CHARACTER cc;
	cc.character.gsm7=tebahpla[ch];
	cc.character.selcted=selected;
	return cc;
}

/*
 * mmcpy (void *destaddr, void const *srcaddr, int len)
 */
void * my_memcpy (void *destaddr, void const *srcaddr, int len)
{
 char *dest = destaddr;
 char const *src = srcaddr;

 while (len-- > 0)
   *dest++ = *src++;
 return destaddr;
}

