/*
 * original author used rawdefines.h to create a private dilect of C++.
 * .pb was short for push-back.
 * .sz was short for size()
 * .mkp was short for make_pair
 ...
*/


#ifndef _RAW_DEFINES_H_
#define _RAW_DEFINES_H_

#define REP(i,N) for(int i = 0;i < (N); ++i )
#define REPV(i,ar) for(int i = 0;i < int( (ar).size() ); ++i )
#define EACH(it,mp) for( __typeof(mp.begin()) it(mp.begin()); it != mp.end(); ++it )
#define FOR(i,a,b) for(int i = (a); i <= (b); ++i )
//#define sz size()
//#define pb push_back
//#define mkp make_pair
//#define INF (int(1e9))
//#define GI ({int t;scanf("%d",&t);t;})
#define EPS (1e-9)
#define EPSILON (1e-9)
//typedef vector<int> VI;
//typedef pair<int,int> PII;
//typedef long long int LL;


#endif
