#include<iostream>
#include<sstream>
#include<string>
#include<vector>
#include<algorithm>
#include<unordered_map>
#include<chrono>
#include<random>
#include<cmath>
#include<variant>
#include<array>
#include<functional>
#include<cstring>
#include<map>
#include<numeric>
#include"jsoncpp/json.h"
using namespace std;
// 这是一份斗地主机器人程序。
// 代码结构大致是：牌型判断 -> 手牌分析 -> 对手猜测 -> 蒙特卡洛搜索 -> 主函数输入输出。
/*斗地主bot --botzone作业
叫分及确定地主
从0号玩家开始，每个玩家轮流选择不叫分或者叫一个比目前为止最高分高的分数（不超过3）。玩家叫3分或2号玩家决策结束后，叫分阶段结束。叫分最高的玩家成为地主，或在没有人叫分的情况下，0号玩家成为地主。

发牌与明牌
每人先发17张牌，留三张牌作为底牌，牌面朝下放在桌上。这三张牌在地主确定后被亮明，所有人都知道这三张牌是什么。然后这三张牌归地主所有。即出牌前地主拥有20张牌，两农民各17张牌。

出牌
游戏开始时由地主先出，可出任一牌型的牌，接着地主的下家农民甲出牌，然后农民甲的下家农民乙先后出牌，之后一直重复地主、农民甲、农民乙的出牌顺序，直到某一方出完手中所有牌。

下家需要出比上家更大的牌，而且除了火箭和炸弹，牌的类型也要与上家相同。如果不出牌则选择“过”，由下一位玩家出牌。如果一玩家出牌后没有其他玩家打出更大的牌，则该玩家获得又一次任意出牌的机会。当一位玩家的手牌数为零时该方获胜，游戏结束。

牌型和大小
牌型有单张、一对、大于等于5张的连牌（顺子）、连对（至少三对）、三张、三张带一张、三张相同牌带一对牌、四张相同牌带任二张单牌或两对，炸弹（四张点数相同的牌），火箭（一对王，也就是一对Joker）。

牌型大小为：

火箭 ＞ 炸弹 ＞ 其他牌型

火箭最大，可以打任何的牌

炸弹按数值比大小，2最大，3最小

单张，一对，三带，单顺，双顺，飞机，四带二等等，全部同等级，但不同牌型之间互相不能混打

异常处理和分数计算
出现非法操作（崩溃、超时等），则该玩家会被裁判托管。托管后的玩家的决策策略是：

叫分阶段叫0分。
出牌阶段如果可以跳过则跳过，否则出序号最小的一张牌。
在被托管的玩家数目达到3的瞬间，游戏结束，所有人获得-1分。

定义底分为 min(地主叫分, 1) ，此后底分在以下特殊情况会进行翻倍：

任何一个玩家出了炸弹，每个炸弹都会使底分翻一倍。
任何一个玩家出了火箭，则会使底分翻一倍。
地主春天，也即两个农民一张牌都没有出，而地主全部出完，底分翻一倍。
农民反春，也即地主除了第一手开牌之外，没有再出过牌，而其中一个农民全部出完，底分翻一倍。
当一名玩家出完所有的牌之后，游戏结束，开始结算分数。

如果农民方胜利，则每个农民获得底分，地主失去两倍底分。
如果地主方胜利，则每个农民失去底分，地主获得两倍底分。
牌型
单张
大王（彩色Joker）>小王（黑白Joker）>2>A>K>Q>J>10>9>8...>3，不计花色，上家出3，下家必须出4或以上。
一对
22>AA>...>33，不计花色，上家出33，必须出44或以上。一对王称为火箭，下详解。
单顺
最少5张牌，不能有2，不计花色。
例如：345678，78910JQ，345678910JQKA
上家出6只，必须跟6只；上家出10只，必须跟10只，如此类推。
双顺
最少3对，不能有2，不计花色。
例如：778899，445566，334455667788991010JJQQ
上家出6对，必须跟6对；上家出3对，必须跟3对，如此类推。
三带
三带分3种，但大前提都是2>A>...>3，并只以三条的部分比大小。
三不带（三带零），即三条，例如222，AAA，666，888等。
三带一，即三条+一只，例如2223，AAAJ，6669等，带的牌不能和三条部分的牌重复，否则会被判为炸弹。
三带二，即三条+一对，例如22233，AAAJJ等。
上家出三带一，必须跟三带一；上家出三不带，必须出三不带。
提醒：所带的单张牌也可以是大王、小王，但是大小王放在一起不算对子。下同。

四带二
四带分为两种，但大前提都是2>A>...>3，并只以四条的部分比大小。
四条加两只（两只不可重复），或四条加两对（两对不可重复），即888857，2222QQAA
如上家出四条加两只必须跟四条加两只，上家出四条加两对必须跟四条加两对。
四带二效果不等同炸弹，只当作普通牌型使用。
飞机
飞机是两个或多个连续数字的三条，只以三条的部分比大小。
飞机分三种，飞机不带翼，飞机带小翼，飞机带大翼。
例如：333444，777888999101010JJJQQQ
飞机不带翼，即纯粹飞机，例如：444555
飞机带小翼，即连续多于一个三带一，所带的单牌不能出现重复，单牌不能和三条部分牌重复，否则会被判为其余牌型，或违规牌型
飞机带大翼，即连续多于一个三带二，所带的对子不能出现重复
例如：33344456，77788834，101010JJJQQQ335577，6667778883399JJ是合法牌型
例如：33344455（违规），33344434（航天飞机），33344435（违规），3334446666（违规）是其余或违规牌型
如上家出飞机不带翼必须跟飞机不带翼，如上家出飞机带翼必须跟飞机带翼
任何情况下，其三条部分都不能有2
航天飞机
此种牌型极少出现，但仍有理论上的可能性。

航天飞机是两个或多个连续数字的四条，只以四条的部分比大小 航天飞机分三种，不带翼，带小翼（各两只），带大翼（各两对）。 同样，所有的只和对均不可重复。

如：

不带翼: 33334444
带小翼: 44445555 37 JQ，333344445555 67 89 10J（18张牌，仅地主）
带大翼: 44445555 3377 JJQQ，33334444 6677 8899
任何情况下，其四条部分都不能有2
炸弹
即四条，如：9999，QQQQ
炸弹大于除火箭外的一切牌型。点数大的炸弹大于点数小的炸弹，如:4444>3333，最大的炸弹是2222
火箭
即大王+小王
火箭大于所有牌型


游戏交互方式
提示
如果你不能理解以下交互方式,可以直接看#游戏样例程序，修改其中

// 做出决策（你只需修改以下部分）
到

// 决策结束，输出结果（你只需修改以上部分）
之间的部分即可！

本游戏与Botzone上其他游戏一样，使用相同的交互方式：Bot#交互

请注意程序是有计算时间的限制的，每步要在1秒内完成！

具体交互内容
在交互中，叫分决策包括0-3共四个整数，分别表示不叫分、叫1分、叫2分、叫3分。

在交互中，游戏中的所有牌使用0-53共54个正整数进行编号。对应关系如下：

牌号	牌面	牌号	牌面	牌号	牌面	牌号	牌面	牌号	牌面	牌号	牌面
0	红桃3	1	方块3	2	黑桃3	3	草花3	4	红桃4	5	方块4
6	黑桃4	7	草花4	8	红桃5	9	方块5	10	黑桃5	11	草花5
12	红桃6	13	方块6	14	黑桃6	15	草花6	16	红桃7	17	方块7
18	黑桃7	19	草花7	20	红桃8	21	方块8	22	黑桃8	23	草花8
24	红桃9	25	方块9	26	黑桃9	27	草花9	28	红桃10	29	方块10
30	黑桃10	31	草花10	32	红桃J	33	方块J	34	黑桃J	35	草花J
36	红桃Q	37	方块Q	38	黑桃Q	39	草花Q	40	红桃K	41	方块K
42	黑桃K	43	草花K	44	红桃A	45	方块A	46	黑桃A	47	草花A
48	红桃2	49	方块2	50	黑桃2	51	草花2	52	小王	53	大王
每回合只有一个Bot会收到request。Bot收到的request是一个JSON对象，表示之前的出牌或叫分情况。格式如下：

叫分request

{
    "own": [0, 1, 2, 3, 4] // 自己最初拥有哪些牌
    "bid": [0, 2] // 前面的玩家的叫分决策
}
对于bid数组：

0号玩家得到空数组
1号玩家得到长度为1的数组，是0号玩家的叫分决策
2号玩家得到长度为2的数组，分别表示0号玩家和1号玩家的叫分决策。
收到叫分request时，Bot所需要输出的response是一个数字，表示自己的叫分决策。

第一个出牌request

{
    "history": [[0, 1, 2] 上上家 , [] 上家 ],  总是两项，每一项都是数组，分别表示上上家和上家出的牌，空数组表示跳过回合或者还没轮到他。
    "publiccard": [29, 8, 14], 地主被公开的三张牌
    "own": [0, 1, 2, 3, 4] // 自己最初拥有哪些牌
    "landlord": 2, // 地主的玩家位置
    "pos": 0, // Bot的玩家位置
    "finalbid": 1 // 叫分阶段的底分
}
此后的出牌request

{
    "history": [[0, 1, 2] 上上家 , [] 上家 ], // 总是两项，每一项都是数组，分别表示上上家和上家出的牌，空数组表示跳过回合。
}
收到出牌request时，Bot所需要输出的response是一个JSON数组，表示自己要出的牌。空数组表示跳过回合。

    作者：舒义鹏，时间：2026-4-7
*/
// 全局随机数引擎（程序启动时初始化，使用硬件熵源）
static mt19937 globalRng(random_device{}());

// 返回 [low, high] 内的均匀随机整数
inline int randomInt(int low, int high) {
    return uniform_int_distribution<int>(low, high)(globalRng);
}

namespace BitOps {
    static inline uint32_t makeMask(const int cnt[18], int need, int beginRank = 3, int endRank = 15) {
        uint32_t mask = 0;
        for (int v = beginRank; v <= endRank; ++v) {
            if (cnt[v] >= need) mask |= (1u << (v - beginRank));
        }
        return mask;
    }

    static inline int popcount32(uint32_t mask) {
        return __builtin_popcount(mask);
    }

    static inline int longestRun(uint32_t mask) {
        int best = 0;
        while (mask) {
            int start = __builtin_ctz(mask);
            uint32_t shifted = mask >> start;
            int len = 0;
            while (shifted & 1u) {
                ++len;
                shifted >>= 1;
            }
            if (len > best) best = len;
            mask >>= start + len;
        }
        return best;
    }

    static inline int longestZeroRun(uint32_t filledMask, int bits) {
        uint32_t allMask = (bits >= 32) ? 0xffffffffu : ((1u << bits) - 1u);
        uint32_t emptyMask = (~filledMask) & allMask;
        return longestRun(emptyMask);
    }

    static inline int selectKthSetBit(uint64_t mask, int k) {
        while (mask) {
            int bit = __builtin_ctzll(mask);
            if (k == 0) return bit;
            mask &= (mask - 1);
            --k;
        }
        return -1;
    }

    static inline uint64_t packCounts(const int cnt[18]) {
        uint64_t packed = 0;
        for (int rank = 0; rank < 18; ++rank) {
            packed |= (uint64_t(cnt[rank] & 7) << (rank * 3));
        }
        return packed;
    }

    static inline void unpackCounts(uint64_t packed, int cnt[18]) {
        for (int rank = 0; rank < 18; ++rank) {
            cnt[rank] = int((packed >> (rank * 3)) & 7ull);
        }
    }

    static inline int getCount(uint64_t packed, int rank) {
        return int((packed >> (rank * 3)) & 7ull);
    }

    static inline uint64_t setCount(uint64_t packed, int rank, int value) {
        uint64_t mask = 7ull << (rank * 3);
        packed &= ~mask;
        packed |= (uint64_t(value & 7) << (rank * 3));
        return packed;
    }

    static inline uint64_t addCount(uint64_t packed, int rank, int delta) {
        int value = getCount(packed, rank) + delta;
        if (value < 0) value = 0;
        if (value > 7) value = 7;
        return setCount(packed, rank, value);
    }

    static inline int totalCount(uint64_t packed) {
        int total = 0;
        for (int rank = 3; rank <= 17; ++rank) total += getCount(packed, rank);
        return total;
    }
}
//==========牌型结构体==========//
// 这些小结构体只是“标签”，表示一种牌型。
// 例如 Single 表示单张，Straight 表示顺子，Rocket 表示火箭。
struct Rocket {};
struct Bomb { int value; };
struct Single { int value; };
struct Pair { int value; };
struct Triple { int value; };
struct Straight { int start; int len; };
struct PairSequence { int start; int len; };
struct TripleSequence { int start; int len; };
struct TripleWithOne { int triple; int single; };
struct TripleWithTwo { int triple; int pair; };
struct QuadWithSingles { int quad; int single1; int single2; };
struct QuadWithPairs { int quad; int pair1; int pair2; };
struct PlanWithSingles { int start; int len; vector<int> singles; };
struct PlanWithPairs { int start; int len; vector<int> pairs; };
using CardPattern = variant<
    Rocket, Bomb, Single, Pair
    , Triple, Straight, PairSequence,
    TripleSequence, TripleWithOne
    , TripleWithTwo, QuadWithSingles,
    QuadWithPairs, PlanWithSingles
    , PlanWithPairs>;
//====牌型检测命名空间===//
// 这一段负责判断“手里的牌到底是什么牌型”，以及“同一种牌里谁更大”。
namespace PatternCheck {
    // 判断一组牌是不是火箭。
    // 输入 cnt 是“点数 -> 张数”的统计表，lastMovePatterns 只在跟牌时使用。
    // exact=true 时要求这 2 张牌正好就是两个王；exact=false 时只要手里能凑出两个王，就认为找到了一个火箭。
    bool check(const int cnt[18], const vector<int>& /*lastMovePatterns*/, Rocket&, bool exact = false) {
        if (exact) {
            int total = 0;
            for (int i = 3; i <= 17; ++i) total += cnt[i];
            return total == 2 && cnt[16] == 1 && cnt[17] == 1;
        }
        else {
            return cnt[16] >= 1 && cnt[17] >= 1;
        }
    }

    // 判断一组牌是不是炸弹，或者在跟牌时是不是更大的炸弹。
    // 返回值表示“能不能组成这种牌型”，同时会把炸弹的点数写回 bomb.value。
    // exact=true 表示必须整整 4 张且没有别的牌；false 表示只要能找到一个比上家大的炸弹就行。
    bool check(const int cnt[18], const vector<int>& lastMovePatterns, Bomb& bomb, bool exact = false) {
        if (exact) {
            int total = 0, quadVal = -1;
            for (int v = 3; v <= 17; ++v) {
                total += cnt[v];
                if (cnt[v] == 4) quadVal = v;
                else if (cnt[v] != 0) return false;
            }
            if (total == 4 && quadVal != -1) {
                bomb.value = quadVal;
                return true;
            }
            return false;
        }
        else {
            for (int v = 3; v <= 17; ++v) {
                if (cnt[v] >= 4) {
                    bomb.value = v;
                    if (!lastMovePatterns.empty() && lastMovePatterns.size() == 4 && v <= lastMovePatterns[0])
                        continue;
                    return true;
                }
            }
            return false;
        }
    }

    // 判断一组牌是不是单张，或者在跟牌时是不是更大的单张。
    // exact=true 时只能有 1 张牌；exact=false 时会从小到大找一个能压过上家的单张。
    bool check(const int cnt[18], const vector<int>& lastMovePatterns, Single& single, bool exact = false) {
        if (exact) {
            int total = 0, val = -1;
            for (int v = 3; v <= 17; ++v) {
                total += cnt[v];
                if (cnt[v] == 1) val = v;
                else if (cnt[v] != 0) return false;
            }
            if (total == 1 && val != -1) {
                single.value = val;
                return true;
            }
            return false;
        }
        else {
            for (int v = 3; v <= 17; ++v) {
                if (cnt[v] >= 1) {
                    if (!lastMovePatterns.empty() && lastMovePatterns[0] >= v) continue;
                    single.value = v;
                    return true;
                }
            }
            return false;
        }
    }

    // 判断一组牌是不是对子，或者在跟牌时是不是更大的对子。
    // 这个函数会把最终找到的对子点数写到 pair.value 里。
    bool check(const int cnt[18], const vector<int>& lastMovePatterns, Pair& pair, bool exact = false) {
        if (exact) {
            int total = 0, val = -1;
            for (int v = 3; v <= 17; ++v) {
                total += cnt[v];
                if (cnt[v] == 2) val = v;
                else if (cnt[v] != 0) return false;
            }
            if (total == 2 && val != -1) {
                pair.value = val;
                return true;
            }
            return false;
        }
        else {
            for (int v = 3; v <= 17; ++v) {
                if (cnt[v] >= 2) {
                    if (!lastMovePatterns.empty() && lastMovePatterns[0] >= v) continue;
                    pair.value = v;
                    return true;
                }
            }
            return false;
        }
    }

    // 判断一组牌是不是三张，或者在跟牌时是不是更大的三张。
    // 三张通常是很多复合牌型的“主干”，后面几类牌型都会先复用这个判断。
    bool check(const int cnt[18], const vector<int>& lastMovePatterns, Triple& triple, bool exact = false) {
        if (exact) {
            int total = 0, val = -1;
            for (int v = 3; v <= 17; ++v) {
                total += cnt[v];
                if (cnt[v] == 3) val = v;
                else if (cnt[v] != 0) return false;
            }
            if (total == 3 && val != -1) {
                triple.value = val;
                return true;
            }
            return false;
        }
        else {
            for (int v = 3; v <= 17; ++v) {
                if (cnt[v] >= 3) {
                    if (!lastMovePatterns.empty() && lastMovePatterns[0] >= v) continue;
                    triple.value = v;
                    return true;
                }
            }
            return false;
        }
    }

    // 判断一组牌是不是顺子。
    // 顺子要求至少 5 张，且只能由连续的单张组成，2 和王不能进顺子。
    bool check(const int cnt[18], const vector<int>& lastMovePatterns, Straight& straight, bool exact = false) {
        if (exact) {
            int total = 0;
            for (int i = 3; i <= 17; ++i) total += cnt[i];
            if (total < 5 || total > 12) return false;
            int start = -1, len = 0;
            for (int i = 3; i <= 14; ++i) {
                if (cnt[i] == 1) {
                    if (start == -1) start = i;
                    len++;
                    if (i == 14 || cnt[i + 1] != 1) break;
                }
                else if (cnt[i] != 0) return false;
            }
            if (len == total && len >= 5) {
                straight.start = start;
                straight.len = len;
                return true;
            }
            return false;
        }
        else {
            int maxlen = 0, beststart = -1;
            for (int start = 3; start <= 14; ++start) {
                int len = 0;
                while (start + len <= 14 && cnt[start + len] >= 1) len++;
                if (len >= 5 && len > maxlen) {
                    maxlen = len;
                    beststart = start;
                }
                start += len;
            }
            if (beststart == -1) return false;
            if (!lastMovePatterns.empty()) {
                int lastlen = lastMovePatterns.size();
                int laststart = lastMovePatterns[0];
                if (maxlen != lastlen || beststart <= laststart) return false;
            }
            straight.start = beststart;
            straight.len = maxlen;
            return true;
        }
    }

    // 判断一组牌是不是连对。
    // 连对就是“连续的对子”，至少 3 个对子起步。
    bool check(const int cnt[18], const vector<int>& lastMovePatterns, PairSequence& ps, bool exact = false) {
        if (exact) {
            int total = 0;
            for (int i = 3; i <= 17; ++i) total += cnt[i];
            int len = total / 2;
            if (total % 2 != 0 || total < 6 || len > 10) return false;
            vector<int> vals;
            for (int i = 3; i <= 17; ++i) {
                if (cnt[i] == 2) vals.push_back(i);
                else if (cnt[i] != 0) return false;
            }
            if ((int)vals.size() != len) return false;
            bool cont = true;
            for (size_t i = 1; i < vals.size(); ++i) {
                if (vals[i] != vals[i - 1] + 1) { cont = false; break; }
            }
            if (cont && len >= 3) {
                ps.start = vals[0];
                ps.len = len;
                return true;
            }
            return false;
        }
        else {
            int maxlen = 0, beststart = -1;
            for (int start = 3; start <= 14; ++start) {
                int len = 0;
                while (start + len <= 14 && cnt[start + len] >= 2) len++;
                if (len >= 3 && len > maxlen) {
                    maxlen = len;
                    beststart = start;
                }
                start += len;
            }
            if (beststart == -1) return false;
            if (!lastMovePatterns.empty()) {
                int lastlen = lastMovePatterns.size() / 2;
                int laststart = lastMovePatterns[0];
                if (maxlen != lastlen || beststart <= laststart) return false;
            }
            ps.start = beststart;
            ps.len = maxlen;
            return true;
        }
    }

    // 判断一组牌是不是飞机，不带翅膀的那种。
    // 这里的“飞机”指连续的三张，不要求带单牌或对子。
    bool check(const int cnt[18], const vector<int>& lastMovePatterns, TripleSequence& ts, bool exact = false) {
        if (exact) {
            int total = 0;
            for (int i = 3; i <= 17; ++i) total += cnt[i];
            if (total % 3 != 0 || total < 6 || total > 12) return false;
            int len = total / 3;
            vector<int> vals;
            for (int i = 3; i <= 17; ++i) {
                if (cnt[i] == 3) vals.push_back(i);
                else if (cnt[i] != 0) return false;
            }
            if ((int)vals.size() != len) return false;
            bool cont = true;
            for (size_t i = 1; i < vals.size(); ++i) {
                if (vals[i] != vals[i - 1] + 1) { cont = false; break; }
            }
            if (cont && len >= 2) {
                ts.start = vals[0];
                ts.len = len;
                return true;
            }
            return false;
        }
        else {
            int maxlen = 0, beststart = -1;
            for (int start = 3; start <= 14; ++start) {
                int len = 0;
                while (start + len <= 14 && cnt[start + len] >= 3) len++;
                if (len >= 2 && len > maxlen) {
                    maxlen = len;
                    beststart = start;
                }
                start += len;
            }
            if (beststart == -1) return false;
            if (!lastMovePatterns.empty()) {
                int lastlen = lastMovePatterns.size() / 3;
                int laststart = lastMovePatterns[0];
                if (maxlen != lastlen || beststart <= laststart) return false;
            }
            ts.start = beststart;
            ts.len = maxlen;
            return true;
        }
    }

    // 判断一组牌是不是“三带一”。
    // 也就是 3 张相同点数的牌，外加 1 张别的牌。
    bool check(const int cnt[18], const vector<int>& lastMovePatterns, TripleWithOne& tws, bool exact = false) {
        if (exact) {
            int total = 0;
            for (int i = 3; i <= 17; ++i) total += cnt[i];
            if (total != 4) return false;
            int tripleVal = -1, singleVal = -1;
            for (int v = 3; v <= 17; ++v) {
                if (cnt[v] == 3) tripleVal = v;
                else if (cnt[v] == 1) singleVal = v;
                else if (cnt[v] != 0) return false;
            }
            if (tripleVal != -1 && singleVal != -1) {
                tws.triple = tripleVal;
                tws.single = singleVal;
                return true;
            }
            return false;
        }
        else {
            for (int v = 3; v <= 17; ++v) {
                if (cnt[v] >= 3) {
                    if (!lastMovePatterns.empty()) {
                        int lastTriple = lastMovePatterns[0];
                        if (v <= lastTriple) continue;
                    }
                    for (int s = 3; s <= 17; ++s) {
                        if (s != v && cnt[s] >= 1) {
                            tws.triple = v;
                            tws.single = s;
                            return true;
                        }
                    }
                }
            }
            return false;
        }
    }

    // 判断一组牌是不是“三带二”。
    // 也就是 3 张相同点数的牌，再加 1 个对子。
    bool check(const int cnt[18], const vector<int>& lastMovePatterns, TripleWithTwo& tws, bool exact = false) {
        if (exact) {
            int total = 0;
            for (int i = 3; i <= 17; ++i) total += cnt[i];
            if (total != 5) return false;
            int tripleVal = -1, pairVal = -1;
            for (int v = 3; v <= 17; ++v) {
                if (cnt[v] == 3) tripleVal = v;
                else if (cnt[v] == 2) pairVal = v;
                else if (cnt[v] != 0) return false;
            }
            if (tripleVal != -1 && pairVal != -1) {
                tws.triple = tripleVal;
                tws.pair = pairVal;
                return true;
            }
            return false;
        }
        else {
            for (int v = 3; v <= 17; ++v) {
                if (cnt[v] >= 3) {
                    if (!lastMovePatterns.empty()) {
                        int lastTriple = lastMovePatterns[0];
                        if (v <= lastTriple) continue;
                    }
                    for (int s = 3; s <= 17; ++s) {
                        if (s != v && cnt[s] >= 2) {
                            tws.triple = v;
                            tws.pair = s;
                            return true;
                        }
                    }
                }
            }
            return false;
        }
    }

    // 判断一组牌是不是“四带二单”。
    // 4 张同点数的牌，再带 2 张散牌。
    bool check(const int cnt[18], const vector<int>& lastMovePatterns, QuadWithSingles& qws, bool exact = false) {
        if (exact) {
            int total = 0;
            for (int i = 3; i <= 17; ++i) total += cnt[i];
            if (total != 6) return false;
            int quadVal = -1;
            vector<int> singles;
            for (int v = 3; v <= 17; ++v) {
                if (cnt[v] == 4) quadVal = v;
                else if (cnt[v] == 1) singles.push_back(v);
                else if (cnt[v] != 0) return false;
            }
            if (quadVal != -1 && singles.size() == 2) {
                qws.quad = quadVal;
                qws.single1 = singles[0];
                qws.single2 = singles[1];
                return true;
            }
            return false;
        }
        else {
            for (int q = 3; q <= 17; ++q) {
                if (cnt[q] >= 4) {
                    if (!lastMovePatterns.empty()) {
                        int lastQuad = lastMovePatterns[0];
                        if (q <= lastQuad) continue;
                    }
                    vector<int> singles;
                    for (int s = 3; s <= 17; ++s) {
                        if (s != q && cnt[s] >= 1) singles.push_back(s);
                    }
                    if (singles.size() >= 2) {
                        qws.quad = q;
                        qws.single1 = singles[0];
                        qws.single2 = singles[1];
                        return true;
                    }
                }
            }
            return false;
        }
    }

    // 判断一组牌是不是“四带二对”。
    // 4 张同点数的牌，再带 2 个对子。
    bool check(const int cnt[18], const vector<int>& lastMovePatterns, QuadWithPairs& qwp, bool exact = false) {
        if (exact) {
            int total = 0;
            for (int i = 3; i <= 17; ++i) total += cnt[i];
            if (total != 8) return false;
            int quadVal = -1;
            vector<int> pairs;
            for (int v = 3; v <= 17; ++v) {
                if (cnt[v] == 4) quadVal = v;
                else if (cnt[v] == 2) pairs.push_back(v);
                else if (cnt[v] != 0) return false;
            }
            if (quadVal != -1 && pairs.size() == 2) {
                qwp.quad = quadVal;
                qwp.pair1 = pairs[0];
                qwp.pair2 = pairs[1];
                return true;
            }
            return false;
        }
        else {
            for (int q = 3; q <= 17; ++q) {
                if (cnt[q] >= 4) {
                    if (!lastMovePatterns.empty()) {
                        int lastQuad = lastMovePatterns[0];
                        if (q <= lastQuad) continue;
                    }
                    vector<int> pairs;
                    for (int s = 3; s <= 17; ++s) {
                        if (s != q && cnt[s] >= 2) pairs.push_back(s);
                    }
                    if (pairs.size() >= 2) {
                        qwp.quad = q;
                        qwp.pair1 = pairs[0];
                        qwp.pair2 = pairs[1];
                        return true;
                    }
                }
            }
            return false;
        }
    }

    // 判断一组牌是不是“飞机带单牌”。
    // 先找连续三张的主干，再从剩余牌里挑同样数量的单牌做翅膀。
    bool check(const int cnt[18], const vector<int>& lastMovePatterns, PlanWithSingles& pws, bool exact = false) {
        if (exact) {
            int total = 0;
            for (int i = 3; i <= 17; ++i) total += cnt[i];
            if (total % 4 != 0 || total < 8) return false;
            int k = total / 4;
            vector<int> triples, singles;
            for (int v = 3; v <= 17; ++v) {
                if (cnt[v] == 3) triples.push_back(v);
                else if (cnt[v] == 1) singles.push_back(v);
                else if (cnt[v] != 0) return false;
            }
            if ((int)triples.size() != k || (int)singles.size() != k) return false;
            // 检查三张部分是否连续
            bool cont = true;
            for (int i = 1; i < k; ++i) {
                if (triples[i] != triples[i - 1] + 1) { cont = false; break; }
            }
            if (!cont) return false;
            // 单牌不能与三张点数重复（已经通过 cnt 检查，因为三张部分占3张，单牌占1张，不会重复）
            pws.start = triples[0];
            pws.len = k;
            pws.singles = singles;
            sort(pws.singles.begin(), pws.singles.end()); // 确保有序
            return true;
        }
        else {
            int maxlen = 0, beststart = -1;
            for (int start = 3; start <= 14; ++start) {
                int len = 0;
                while (start + len <= 14 && cnt[start + len] >= 3) len++;
                if (len >= 2 && len > maxlen) {
                    maxlen = len;
                    beststart = start;
                }
                start += len;
            }
            if (beststart == -1) return false;
            if (!lastMovePatterns.empty()) {
                int lastlen = lastMovePatterns.size() / 4;
                int laststart = lastMovePatterns[0];
                if (maxlen != lastlen || beststart <= laststart) return false;
            }
            int tmpCnt[18];
            memcpy(tmpCnt, cnt, sizeof(tmpCnt));
            for (int j = 0; j < maxlen; ++j) tmpCnt[beststart + j] -= 3;
            vector<int> singles;
            for (int v = 3; v <= 17; ++v) {
                for (int k = 0; k < tmpCnt[v]; ++k) singles.push_back(v);
            }
            if ((int)singles.size() < maxlen) return false;
            sort(singles.begin(), singles.end());
            pws.start = beststart;
            pws.len = maxlen;
            pws.singles.assign(singles.begin(), singles.begin() + maxlen);
            return true;
        }
    }

    // 判断一组牌是不是“飞机带对子”。
    // 逻辑和飞机带单牌一样，只是翅膀从单牌换成对子。
    bool check(const int cnt[18], const vector<int>& lastMovePatterns, PlanWithPairs& pwp, bool exact = false) {
        if (exact) {
            int total = 0;
            for (int i = 3; i <= 17; ++i) total += cnt[i];
            if (total % 5 != 0 || total < 10) return false;
            int k = total / 5;
            vector<int> triples, pairs;
            for (int v = 3; v <= 17; ++v) {
                if (cnt[v] == 3) triples.push_back(v);
                else if (cnt[v] == 2) pairs.push_back(v);
                else if (cnt[v] != 0) return false;
            }
            if ((int)triples.size() != k || (int)pairs.size() != k) return false;
            bool cont = true;
            for (int i = 1; i < k; ++i) {
                if (triples[i] != triples[i - 1] + 1) { cont = false; break; }
            }
            if (!cont) return false;
            pwp.start = triples[0];
            pwp.len = k;
            pwp.pairs = pairs;
            sort(pwp.pairs.begin(), pwp.pairs.end());
            return true;
        }
        else {
            int maxlen = 0, beststart = -1;
            for (int start = 3; start <= 14; ++start) {
                int len = 0;
                while (start + len <= 14 && cnt[start + len] >= 3) len++;
                if (len >= 2 && len > maxlen) {
                    maxlen = len;
                    beststart = start;
                }
                start += len;
            }
            if (beststart == -1) return false;
            if (!lastMovePatterns.empty()) {
                int lastlen = lastMovePatterns.size() / 5;
                int laststart = lastMovePatterns[0];
                if (maxlen != lastlen || beststart <= laststart) return false;
            }
            int tmpCnt[18];
            memcpy(tmpCnt, cnt, sizeof(tmpCnt));
            for (int j = 0; j < maxlen; ++j) tmpCnt[beststart + j] -= 3;
            vector<int> pairs;
            for (int v = 3; v <= 17; ++v) {
                for (int k = 0; k < tmpCnt[v] / 2; ++k) pairs.push_back(v);
            }
            if ((int)pairs.size() < maxlen) return false;
            sort(pairs.begin(), pairs.end());
            pwp.start = beststart;
            pwp.len = maxlen;
            pwp.pairs.assign(pairs.begin(), pairs.begin() + maxlen);
            return true;
        }
    }
    // 从“上家出的牌”里找主值。
    // 比如单张/对子/三张时，主值就是那张牌的点数；顺子、连对、飞机时，主值就是起始点数。
    // 这个值专门给“比较大小”和“枚举更大的牌”使用。
    int getMainValueOfLastMove(const vector<int>& lastMovePatterns) {
         if (lastMovePatterns.empty()) return 0;
    int cnt[18] = {0};
    for (int v : lastMovePatterns) cnt[v]++;

    int sz = (int)lastMovePatterns.size();

    // 单张/对子/三条
    if (sz == 1 || sz == 2 || sz == 3)
        return lastMovePatterns[0];

    // 火箭
    if (sz == 2 && cnt[16] == 1 && cnt[17] == 1) return 16;

    // 三带一 (sz=4) 或 三带二 (sz=5)
    if (sz == 4 || sz == 5) {
        for (int v = 3; v <= 17; ++v)
            if (cnt[v] == 3) return v;   // 返回三条的点数
        // 如果是炸弹 (cnt[v]==4, 没有>=3但未满足) 则返回首元素
        return lastMovePatterns[0];
    }

    // 四带二
    if (sz == 6 || sz == 8) {
        for (int v = 3; v <= 17; ++v)
            if (cnt[v] >= 4) return v;
    }

         

        // 连对：sz是偶数，每张2且连续
        if (sz % 2 == 0) {
            bool isPairSeq = true;
            int startPS = -1, lenPS = 0;
            for (int v = 3; v <= 14; ++v) {
                if (cnt[v] == 2) {
                    if (startPS == -1) startPS = v;
                    lenPS++;
                }
                else if (cnt[v] != 0) { isPairSeq = false; break; }
            }
            if (isPairSeq && lenPS == sz / 2) return startPS;
        }

        // 飞机无翼：sz是3的倍数，每张3且连续
        if (sz % 3 == 0) {
            bool isTriSeq = true;
            int startTS = -1, lenTS = 0;
            for (int v = 3; v <= 14; ++v) {
                if (cnt[v] == 3) {
                    if (startTS == -1) startTS = v;
                    lenTS++;
                }
                else if (cnt[v] != 0) { isTriSeq = false; break; }
            }
            int k = sz / 3;
            if (isTriSeq && lenTS == k) return startTS;
        }

         
        // 顺子：sz张，都是1张且连续
        bool isStraight = true;
        int startStraight = -1, lenStraight = 0;
        for (int v = 3; v <= 14; ++v) {
            if (cnt[v] == 1) {
                if (startStraight == -1) startStraight = v;
                lenStraight++;
            }
            else if (cnt[v] > 1) { isStraight = false; break; }
        }
        if (isStraight && lenStraight == sz) return startStraight;
        // 飞机带单 (sz = 4k, k>=2) 或飞机带对 (sz = 5k, k>=2)
        if ((sz >= 8 && sz % 4 == 0) || (sz >= 10 && sz % 5 == 0)) {
            int seqStart = -1, seqLen = 0;
            for (int v = 3; v <= 14; ++v) {
                if (cnt[v] >= 3) {
                    if (seqStart == -1) seqStart = v;
                    seqLen++;
                }
                else {
                    if (seqLen >= 2) break;
                    seqStart = -1;
                    seqLen = 0;
                }
            }
            if (seqStart != -1 && seqLen >= 2) return seqStart;
        }

        // fallback
        return lastMovePatterns[0];
    }
    // 枚举火箭：如果同时有两个王，就只能得到一种答案。
    inline vector<vector<int>> enumerateRocket(const int cnt[18]) {
        if (cnt[16] >= 1 && cnt[17] >= 1) return { {16, 17} };
        return {};
    }
    // 枚举炸弹：返回所有能出的四张同点数组合。
    // 如果是跟炸弹，还会只保留比上家更大的炸弹。
    inline vector<vector<int>> enumerateBombs(const int cnt[18], const vector<int>& lastMovePatterns) {
        vector<vector<int>> result;
        int minVal = 3;
        if (!lastMovePatterns.empty() && lastMovePatterns.size() == 4)
            minVal = getMainValueOfLastMove(lastMovePatterns)+1;
        for (int v = minVal; v <= 17; ++v) {
            if (cnt[v] >= 4) result.push_back({ v, v, v, v });
        }
        return result;
    }
    // 枚举单张：从小到大把每一个能出的点数都列出来。
    inline vector<vector<int>> enumerateSingles(const int cnt[18], const vector<int>& lastMovePatterns) {
        vector<vector<int>> result;
        int minVal = 3;
        if (!lastMovePatterns.empty() && lastMovePatterns.size() == 1)
            minVal = getMainValueOfLastMove(lastMovePatterns)+1;
        for (int v = minVal; v <= 17; ++v) {
            if (cnt[v] >= 1) result.push_back({ v });
        }
        return result;
    }
    // 枚举对子：把所有能组成对子、且能压过上家的对子都列出来。
    inline vector<vector<int>> enumeratePairs(const int cnt[18], const vector<int>& lastMovePatterns) {
        vector<vector<int>> result;
        int minVal = 3;
        if (!lastMovePatterns.empty() && lastMovePatterns.size() == 2)
            minVal = getMainValueOfLastMove(lastMovePatterns)+1;
        for (int v = minVal; v <= 17; ++v) {
            if (cnt[v] >= 2) result.push_back({ v, v });
        }
        return result;
    }
    // 枚举三张：把所有三张牌型都找出来。
    inline vector<vector<int>> enumerateTriples(const int cnt[18], const vector<int>& lastMovePatterns) {
        vector<vector<int>> result;
        int minVal = 3;
        if (!lastMovePatterns.empty() && lastMovePatterns.size() == 3)
            minVal = lastMovePatterns[0] + 1;
        for (int v = minVal; v <= 17; ++v) {
            if (cnt[v] >= 3) result.push_back({ v, v, v });
        }
        return result;
    }
    // 枚举顺子：找出所有连续单张组合。
    inline vector<vector<int>> enumerateStraights(const int cnt[18], const vector<int>& lastMovePatterns) {
        vector<vector<int>> result;
        int lastLen = 0, lastStart = 0;
        if (!lastMovePatterns.empty()) {
            lastLen = lastMovePatterns.size();
            lastStart = getMainValueOfLastMove(lastMovePatterns)+1;
        }
        for (int len = 5; len <= 12; ++len) {
            if (lastLen != 0 && len != lastLen) continue;
            for (int start = 3; start + len - 1 <= 14; ++start) {
                if (lastLen != 0 && start <= lastStart) continue;
                bool ok = true;
                for (int i = 0; i < len; ++i) {
                    if (cnt[start + i] < 1) { ok = false; break; }
                }
                if (ok) {
                    vector<int> straight;
                    for (int i = 0; i < len; ++i) straight.push_back(start + i);
                    result.push_back(straight);
                }
            }
        }
        return result;
    }
    inline vector<vector<int>> enumeratePairSequences(const int cnt[18], const vector<int>& lastMovePatterns) {
        vector<vector<int>> result;
        int lastLen = 0, lastStart = 0;
        if (!lastMovePatterns.empty()) {
            lastLen = lastMovePatterns.size() / 2;
            lastStart = getMainValueOfLastMove(lastMovePatterns)+1;
        }
        for (int len = 3; len <= 12; ++len) {
            if (lastLen != 0 && len != lastLen) continue;
            for (int start = 3; start + len - 1 <= 14; ++start) {
                if (lastLen != 0 && start <= lastStart) continue;
                bool ok = true;
                for (int i = 0; i < len; ++i) {
                    if (cnt[start + i] < 2) { ok = false; break; }
                }
                if (ok) {
                    vector<int> seq;
                    for (int i = 0; i < len; ++i) {
                        seq.push_back(start + i);
                        seq.push_back(start + i);
                    }
                    result.push_back(seq);
                }
            }
        }
        return result;
    }
    inline vector<vector<int>> enumerateTripleSequence(const int cnt[18], const vector<int>& lastMovePatterns) {
        vector<vector<int>> result;
        int lastLen = 0, lastStart = 0;
        if (!lastMovePatterns.empty()) {
            lastLen = lastMovePatterns.size() / 3;
            lastStart = getMainValueOfLastMove(lastMovePatterns)+1;
        }
        for (int len = 2; len <= 6; ++len) {
            if (lastLen != 0 && len != lastLen) continue;
            for (int start = 3; start + len - 1 <= 14; ++start) {
                if (lastLen != 0 && start <= lastStart) continue;
                bool ok = true;
                for (int i = 0; i < len; ++i) {
                    if (cnt[start + i] < 3) { ok = false; break; }
                }
                if (ok) {
                    vector<int> plane;
                    for (int i = 0; i < len; ++i) {
                        for (int k = 0; k < 3; ++k) plane.push_back(start + i);
                    }
                    result.push_back(plane);
                }
            }
        }
        return result;
    }
    inline vector<vector<int>> enumerateTriplesWithOne(const int cnt[18], const vector<int>& lastMovePatterns) {
        vector<vector<int>> result;
        int minTriple = 3;
        if (!lastMovePatterns.empty() && lastMovePatterns.size() == 4)
            minTriple = getMainValueOfLastMove(lastMovePatterns)+1;
        for (int triple = minTriple; triple <= 17; ++triple) {
            if (cnt[triple] < 3) continue;
            for (int single = 3; single <= 17; ++single) {
                if (single == triple) continue;
                if (cnt[single] < 1) continue;
                result.push_back({ triple, triple, triple, single });
            }
        }
        return result;
    }
    inline vector<vector<int>> enumerateTriplesWithTwo(const int cnt[18], const vector<int>& lastMovePatterns) {
        vector<vector<int>> result;
        int minTriple = 3;
        if (!lastMovePatterns.empty() && lastMovePatterns.size() == 5)
            minTriple = getMainValueOfLastMove(lastMovePatterns)+1;
        for (int triple = minTriple; triple <= 17; ++triple) {
            if (cnt[triple] < 3) continue;
            for (int pair = 3; pair <= 17; ++pair) {
                if (pair == triple) continue;
                if (cnt[pair] < 2) continue;
                result.push_back({ triple, triple, triple, pair, pair });
            }
        }
        return result;
    }
    inline vector<vector<int>> enumeratePlanesWithSingles(const int cnt[18], const vector<int>& lastMovePatterns) {
        vector<vector<int>> result;
        int lastLen = 0, lastStart = 0;
        if (!lastMovePatterns.empty()) {
            lastLen = lastMovePatterns.size() / 4;
            lastStart = getMainValueOfLastMove(lastMovePatterns);
        }
        for (int len = 2; len <= 4; ++len) {
            if (lastLen != 0 && len != lastLen) continue;
            for (int start = 3; start + len - 1 <= 14; ++start) {
                if (lastLen != 0 && start <= lastStart) continue;
                bool ok = true;
                for (int i = 0; i < len; ++i) {
                    if (cnt[start + i] < 3) { ok = false; break; }
                }
                if (!ok) continue;
                // 收集可用单牌（不能与飞机点数重复）
                vector<int> singles;
                for (int v = 3; v <= 17; ++v) {
                    bool used = false;
                    for (int i = 0; i < len; ++i) if (start + i == v) { used = true; break; }
                    if (used) continue;
                    if (cnt[v] >= 1) singles.push_back(v);
                }
                if ((int)singles.size() < len) continue;
                // 枚举所有组合（C(singles.size(), len)），为了性能，只取最小的len个单牌作为代表
                // 但为了完整性，这里实现简单的递归枚举（实际手牌中单牌数量有限，开销不大）
                vector<int> planeBody;
                for (int i = 0; i < len; ++i)
                    for (int k = 0; k < 3; ++k) planeBody.push_back(start + i);
                if (singles.size() >= len) {
                    std::sort(singles.begin(), singles.end());
                    singles.resize(len);
                    vector<int> action = planeBody;
                    action.insert(action.end(), singles.begin(), singles.end());
                    result.push_back(action);
                }
            }
        }
        return result;
    }
    inline vector<vector<int>> enumeratePlanesWithPairs(const int cnt[18], const vector<int>& lastMovePatterns) {
        vector<vector<int>> result;
        int lastLen = 0, lastStart = 0;
        if (!lastMovePatterns.empty()) {
            lastLen = lastMovePatterns.size() / 5;
            lastStart = getMainValueOfLastMove(lastMovePatterns);
        }
        for (int len = 2; len <= 4; ++len) {
            if (lastLen != 0 && len != lastLen) continue;
            for (int start = 3; start + len - 1 <= 14; ++start) {
                if (lastLen != 0 && start <= lastStart) continue;
                bool ok = true;
                for (int i = 0; i < len; ++i) {
                    if (cnt[start + i] < 3) { ok = false; break; }
                }
                if (!ok) continue;
                // 收集可用对子（不能与飞机点数重复）
                vector<int> pairs;
                for (int v = 3; v <= 17; ++v) {
                    bool used = false;
                    for (int i = 0; i < len; ++i) if (start + i == v) { used = true; break; }
                    if (used) continue;
                    if (cnt[v] >= 2) pairs.push_back(v);
                }
                if ((int)pairs.size() < len) continue;
                // 枚举所有组合
                vector<int> planeBody;
                for (int i = 0; i < len; ++i)
                    for (int k = 0; k < 3; ++k) planeBody.push_back(start + i);
                if (pairs.size() >= len) {
                    std::sort(pairs.begin(), pairs.end());
                    pairs.resize(len);   // 只保留最小的 len 个对子

                    vector<int> action = planeBody;
                    for (int v : pairs) {
                        action.push_back(v);
                        action.push_back(v);
                    }
                    result.push_back(action);
                }
            }
        }
        return result;
    }
    inline vector<vector<int>> enumerateQuadWithSingles(const int cnt[18], const vector<int>& lastMovePatterns) {
        vector<vector<int>> result;
        int minQuad = 3;
        if (!lastMovePatterns.empty() && lastMovePatterns.size() == 6) {
            minQuad = getMainValueOfLastMove(lastMovePatterns)+1;
        }
        for (int quad = minQuad; quad <= 17; ++quad) {
            if (cnt[quad] < 4) continue;
            vector<int> singles;
            for (int s = 3; s <= 17; ++s) {
                if (s != quad && cnt[s] >= 1) singles.push_back(s);
            }
            if (singles.size() < 2) continue;
            if (singles.size() >= 2) {
                std::sort(singles.begin(), singles.end());
                vector<int> action(4, quad);
                action.push_back(singles[0]);
                action.push_back(singles[1]);
                result.push_back(action);
            }
        }
        return result;
    }
    inline vector<vector<int>> enumerateQuadWithPairs(const int cnt[18], const vector<int>& lastMovePatterns) {
        vector<vector<int>> result;
        int minQuad = 3;
        if (!lastMovePatterns.empty() && lastMovePatterns.size() == 8) {
            minQuad = getMainValueOfLastMove(lastMovePatterns)+1;
        }
        for (int quad = minQuad; quad <= 17; ++quad) {
            if (cnt[quad] < 4) continue;
            vector<int> pairs;
            for (int p = 3; p <= 17; ++p) {
                if (p != quad && cnt[p] >= 2) pairs.push_back(p);
            }
            if (pairs.size() < 2) continue;
            if (pairs.size() >= 2) {
                std::sort(pairs.begin(), pairs.end());
                vector<int> action(4, quad);
                action.push_back(pairs[0]); action.push_back(pairs[0]);
                action.push_back(pairs[1]); action.push_back(pairs[1]);
                result.push_back(action);
            }
        }
        return result;
    }
     
}
class CardPatternAnalysis {
private:
    static const  vector<int> reserve_value_table[18];
public:
    static constexpr double kBidFirstMultiplier = 1.75;
    static constexpr double kBidLastMultiplier = 0.95;
    static constexpr double kBidAssistPenalty = 1.05;
    static constexpr double kBidLevel1Threshold = 14.0;
    static constexpr double kBidLevel2Threshold = 28.0;
    static constexpr double kBidLevel3Threshold = 60.0;
    static constexpr double kBidForce3Threshold = 70.0;
    static constexpr double kBidLastOpenThreshold = 17.0;

    // 这些常量给不同牌型编号，后面比较和分支时会用到。
    static const int SINGLE = 0;
    static const int PAIR = 1;
    static const int TRIPLE = 2;
    static const int THREE_WITH_ONE = 3;
    static const int THREE_WITH_TWO = 4;
    static const int TRIPLE_SEQUENCE = 7;
    static const int TRIPLE_SEQUENCE_WITH_TWO_PAIRS = 6;
    static const int TRIPLE_SEQUENCE_WITH_ONE = 5;
    static const int STRAIGHT = 9;
    static const int PAIR_SEQUENCE = 8;
    static const int BOMB = 10;
    static const int ROCKET = 11;
    static const int QUAD_WITH_SINGLES = 12;
    static const int QUAD_WITH_PAIRS = 13;
    // 看一组牌属于哪一种牌型。
    static int getCardType(const vector<int>& cards) {
        vector<int> patterns = divideIntoPatterns(cards);
        int cnt[18] = { 0 };
        for (int v : patterns) cnt[v]++;
        vector<int> emptyLast;  // 主动出牌时没有上家牌
        Rocket r;
        if (PatternCheck::check(cnt, emptyLast, r, true)) return ROCKET;
        Bomb b;
        if (PatternCheck::check(cnt, emptyLast, b, true)) return BOMB;
        Single s;
        if (PatternCheck::check(cnt, emptyLast, s, true)) return SINGLE;
        Pair p;
        if (PatternCheck::check(cnt, emptyLast, p, true)) return PAIR;
        Triple t;
        if (PatternCheck::check(cnt, emptyLast, t, true)) return TRIPLE;
        Straight st;
        if (PatternCheck::check(cnt, emptyLast, st, true)) return STRAIGHT;
        PairSequence ps;
        if (PatternCheck::check(cnt, emptyLast, ps, true)) return PAIR_SEQUENCE;
        TripleSequence ts;
        if (PatternCheck::check(cnt, emptyLast, ts, true)) return TRIPLE_SEQUENCE;
        TripleWithOne two1;
        if (PatternCheck::check(cnt, emptyLast, two1, true)) return THREE_WITH_ONE;
        TripleWithTwo two2;
        if (PatternCheck::check(cnt, emptyLast, two2, true)) return THREE_WITH_TWO;
        QuadWithSingles qws;
        if (PatternCheck::check(cnt, emptyLast, qws, true)) return QUAD_WITH_SINGLES;
        QuadWithPairs qwp;
        if (PatternCheck::check(cnt, emptyLast, qwp, true)) return QUAD_WITH_PAIRS;
        PlanWithSingles pws;
        if (PatternCheck::check(cnt, emptyLast, pws, true)) return TRIPLE_SEQUENCE_WITH_ONE;
        PlanWithPairs pwp;
        if (PatternCheck::check(cnt, emptyLast, pwp, true)) return TRIPLE_SEQUENCE_WITH_TWO_PAIRS;
        return -1;
    }
    // 和 getCardType 类似，但输入的是“点数序列”。
    static int getCardTypeFromPoints(const vector<int>& cards) {
         
        int cnt[18] = { 0 };
        for (int v : cards) cnt[v]++;
        vector<int> emptyLast;  // 主动出牌时没有上家牌
        Rocket r;
        if (PatternCheck::check(cnt, emptyLast, r, true)) return ROCKET;
        Bomb b;
        if (PatternCheck::check(cnt, emptyLast, b, true)) return BOMB;
        Single s;
        if (PatternCheck::check(cnt, emptyLast, s, true)) return SINGLE;
        Pair p;
        if (PatternCheck::check(cnt, emptyLast, p, true)) return PAIR;
        Triple t;
        if (PatternCheck::check(cnt, emptyLast, t, true)) return TRIPLE;
        Straight st;
        if (PatternCheck::check(cnt, emptyLast, st, true)) return STRAIGHT;
        PairSequence ps;
        if (PatternCheck::check(cnt, emptyLast, ps, true)) return PAIR_SEQUENCE;
        TripleSequence ts;
        if (PatternCheck::check(cnt, emptyLast, ts, true)) return TRIPLE_SEQUENCE;
        TripleWithOne two1;
        if (PatternCheck::check(cnt, emptyLast, two1, true)) return THREE_WITH_ONE;
        TripleWithTwo two2;
        if (PatternCheck::check(cnt, emptyLast, two2, true)) return THREE_WITH_TWO;
        QuadWithSingles qws;
        if (PatternCheck::check(cnt, emptyLast, qws, true)) return QUAD_WITH_SINGLES;
        QuadWithPairs qwp;
        if (PatternCheck::check(cnt, emptyLast, qwp, true)) return QUAD_WITH_PAIRS;
        PlanWithSingles pws;
        if (PatternCheck::check(cnt, emptyLast, pws, true)) return TRIPLE_SEQUENCE_WITH_ONE;
        PlanWithPairs pwp;
        if (PatternCheck::check(cnt, emptyLast, pwp, true)) return TRIPLE_SEQUENCE_WITH_TWO_PAIRS;
        return -1;
    }
    // 根据点数，从手牌里找到对应的真实牌号。
    static vector<int> findCardValue(const vector<int>& handcards, const vector<int>& values) {
        vector<int> result;
        vector<int> tempHand = handcards; // 拷贝一份，用于移除已取牌
        for (int val : values) {
            // 在 tempHand 中找第一张点数为 val 的牌
            auto it = find_if(tempHand.begin(), tempHand.end(), [val](int card) {
                return getCardValue(card) == val;
                });
            if (it == tempHand.end()) {
                // 如果找不到任何一张点数为 val 的牌，返回空（表示取牌失败）
                return {};
            }
            result.push_back(*it);
            tempHand.erase(it); // 移除已使用的牌，避免后续重复
        }
        return result;
    }
    // 看手牌是不是已经空了。
    static bool isallout(const vector<int>& cards) {
        return cards.empty();
    }
    // 把牌号变成点数，比如 0~3 变成 3，52 变成小王，53 变成大王。
    static  int getCardValue(int card) {
        int res = 0;
        if (card >= 0 && card <= 3)res = 3;
        else if (card >= 4 && card <= 7)res = 4;
        else if (card >= 8 && card <= 11)res = 5;
        else if (card >= 12 && card <= 15)res = 6;
        else if (card >= 16 && card <= 19)res = 7;
        else if (card >= 20 && card <= 23)res = 8;
        else if (card >= 24 && card <= 27)res = 9;
        else if (card >= 28 && card <= 31)res = 10;
        else if (card >= 32 && card <= 35)res = 11;
        else if (card >= 36 && card <= 39)res = 12;
        else if (card >= 40 && card <= 43)res = 13;
        else if (card >= 44 && card <= 47)res = 14;
        else if (card >= 48 && card <= 51)res = 15;
        else if (card == 52)res = 16; // 小王
        else if (card == 53)res = 17; // 大王
        return res;
    }
    // 把牌号变成点数，并按从小到大排好。
    static vector<int> divideIntoPatterns(const vector<int>& cards) {
        vector<int> patterns;
        for (int card : cards) {
            patterns.push_back(getCardValue(card));
        }
        sort(patterns.begin(), patterns.end());
        return patterns;
    }
    // 统计一手牌里每个点数有几张。
    static void analyzeHand(const vector<int>& hand, int cnt[18], vector<int>& uniqueVals) {
        memset(cnt, 0, sizeof(int) * 18);
        for (int card : hand) {
            int val = getCardValue(card);
            cnt[val]++;
        }
        uniqueVals.clear();
        for (int v = 3; v <= 17; ++v) {
            if (cnt[v] > 0) uniqueVals.push_back(v);
        }
    }
    // 给一整手牌打分，分数越高说明这手牌通常越顺。
    static int evaluateHand(const vector<int>& hand) {
        int cnt[18] = { 0 };
        double score = 0;
        for (int card : hand) {
            int val = CardPatternAnalysis::getCardValue(card);
            cnt[val]++;

        }
        return evaluateHand(cnt);
    }
    // 给“点数统计表”打分。
    static int evaluateHand(const int* cnt) {
        double score = 0;

        // 1. 基础点数（按角色通用权重，可微调）
        score += cnt[17] * 12;   // 大王
        score += cnt[16] * 10;   // 小王
        score += cnt[15] * 6;    // 2
        score += cnt[14] * 4.5;  // A
        score += cnt[13] * 3;    // K
        score += cnt[12] * 2;    // Q
        score += cnt[11] * 1.2;  // J
        score += cnt[10] * 0.8;  // 10
        score += cnt[9]  * 0.4;  // 9
        score += cnt[8]  * 0.2;  // 8
        // 7以下不计基础分，仅靠牌型

        // 2. 炸弹 & 火箭
        uint32_t quadMask = BitOps::makeMask(cnt, 4, 3, 15);
        score += BitOps::popcount32(quadMask) * 18.0;
        if (cnt[16] && cnt[17]) score += 10; // 火箭额外加分（已在基础分中）

        // 3. 顺子检测（最长顺子，长度≥5）
        uint32_t straightMask = BitOps::makeMask(cnt, 1, 3, 14);
        int maxStraight = BitOps::longestRun(straightMask);
        if (maxStraight >= 5) score += maxStraight * 6;

        // 4. 连对检测（最长连对，长度≥3）
        uint32_t pairSeqMask = BitOps::makeMask(cnt, 2, 3, 14);
        int maxPairSeq = BitOps::longestRun(pairSeqMask);
        if (maxPairSeq >= 3) score += maxPairSeq * 5;

        // 5. 三顺（飞机无翼）检测（长度≥2）
        uint32_t tripleSeqMask = BitOps::makeMask(cnt, 3, 3, 14);
        int maxTripleSeq = BitOps::longestRun(tripleSeqMask);
        if (maxTripleSeq >= 2) score += maxTripleSeq * 8;

        // 6. 三条、对子数量（不重复计算已经计入飞机、连对的）
        int tripleCount = BitOps::popcount32(BitOps::makeMask(cnt, 3, 3, 15));
        score += tripleCount * 2.5;

        int pairCount = BitOps::popcount32(BitOps::makeMask(cnt, 2, 3, 15));
        score += pairCount * 1.2;

        // 7. 单牌数量惩罚（散牌扣分）
        uint32_t exactSingleMask = BitOps::makeMask(cnt, 1, 3, 15) & ~BitOps::makeMask(cnt, 2, 3, 15);
        int singles = BitOps::popcount32(exactSingleMask);
        if (cnt[16] == 1) ++singles;
        if (cnt[17] == 1) ++singles;
        score -= singles * 2.5;  // 每张单牌扣分

        // 8. 断点惩罚：连续无牌的区间长度，断带越多牌越不整
        uint32_t filledMask = BitOps::makeMask(cnt, 1, 3, 14);
        int gapLen = 0, totalGap = 0;
        for (int bit = 0; bit < 12; ++bit) {
            if ((filledMask & (1u << bit)) == 0) {
                ++gapLen;
            } else {
                if (gapLen > 2) totalGap += gapLen * 0.8;
                gapLen = 0;
            }
        }
        if (gapLen > 2) totalGap += gapLen * 0.8;
        score -= totalGap * 1.5;

        // 9. 如果手牌总数很少，说明接近胜利，加分（但evaluate通常用于静态评估，这里可忽略）
        int totalCards = 0;
        for (int v = 3; v <= 17; ++v) totalCards += cnt[v];
        if (totalCards <= 4) score += 10;   // 残局优势

        return (int)score;
    }
    static int evaluateAssistPotential(const vector<int>& hand) {
        int cnt[18] = {0};
        for (int c : hand) {
            cnt[CardPatternAnalysis::getCardValue(c)]++;
        }
        int assist = 0;
        // 对子数量 → 连对潜力
        int pairCount = BitOps::popcount32(BitOps::makeMask(cnt, 2, 3, 15));
        assist += pairCount * 2;
        // 三条连续长度（飞机潜力）
        int maxTripleSeq = BitOps::longestRun(BitOps::makeMask(cnt, 3, 3, 14));
        assist += maxTripleSeq * 3;
        // 顺子潜力
        int maxStraight = BitOps::longestRun(BitOps::makeMask(cnt, 1, 3, 14));
        assist += maxStraight;
        return assist;
    }

    // 模拟成为地主后的平均手牌评分
    static double simulateLandlordScore(const vector<int>& hand) {
        int cnt[18] = {0};
        for (int c : hand) {
            cnt[CardPatternAnalysis::getCardValue(c)]++;
        }
        // 构建未知牌池
        vector<int> pool;
        for (int v = 3; v <= 15; ++v)
            for (int i = 0; i < 4 - cnt[v]; ++i) pool.push_back(v);
        for (int i = 0; i < 1 - cnt[16]; ++i) pool.push_back(16);
        for (int i = 0; i < 1 - cnt[17]; ++i) pool.push_back(17);
        if (pool.size() < 3) return evaluateHand(hand); // 极端情况

        const int SIMS = 200;
        long long sum = 0;
        for (int t = 0; t < SIMS; ++t) {
            vector<int> copy = pool;
            std::shuffle(copy.begin(), copy.end(), globalRng);
            int tmpCnt[18];
            memcpy(tmpCnt, cnt, sizeof(tmpCnt));
            for (int i = 0; i < 3; ++i) tmpCnt[copy[i]]++;
            sum += evaluateHand(tmpCnt);
        }
        return (double)sum / SIMS;
    }

    // 改进的叫分决策
    static int decideBid(const vector<int>& hand, int currentMaxBid, bool isLast, bool isFirst) {
        int baseScore = evaluateHand(hand);
        int assistPotential = evaluateAssistPotential(hand);

        // 位置修正系数（首家更保守，末家只略微放宽）
        double thresholdMult = 1.0;
        if (isFirst) {
            thresholdMult = kBidFirstMultiplier;   // 首家门槛提高更多，避免无脑冲 3
        } else if (isLast) {
            thresholdMult = kBidLastMultiplier;    // 末家仍可适度跟进，但不再过于激进
        }
        // 第二家 (中间) 保持 1.0

        // 模拟地主期望评分
        double avgLandlordScore = simulateLandlordScore(hand);

        // 最终叫分意愿评分：地主期望评分减去助攻潜力惩罚（助攻越高越不想当地主）
        double finalScore = avgLandlordScore - assistPotential * kBidAssistPenalty;

           cerr << "[BID DEBUG] handSize=" << hand.size()
               << " baseScore=" << baseScore
               << " assistPotential=" << assistPotential
               << " avgLandlordScore=" << avgLandlordScore
               << " finalScore=" << finalScore
               << " currentMaxBid=" << currentMaxBid
               << " isFirst=" << isFirst
               << " isLast=" << isLast
               << endl;

        // 动态阈值（经过位置调整）
        int idealBid = 0;
        if (finalScore >= kBidLevel3Threshold * thresholdMult) idealBid = 3;
        else if (finalScore >= kBidLevel2Threshold * thresholdMult) idealBid = 2;
        else if (finalScore >= kBidLevel1Threshold * thresholdMult) idealBid = 1;

        // 特殊规则：如果当前无人叫更高分，且手牌极强，可以叫 3 分
        if (idealBid == 0) return 0;

        if (idealBid > currentMaxBid && idealBid <= 3) return idealBid;

        // 如果当前最高叫分小于 3 且评分极高，仍可叫 3 分
        if (currentMaxBid < 3 && finalScore >= kBidForce3Threshold * thresholdMult) return 3;

        // 末家且当前无人叫分，且手牌尚可，可叫 1 分抢地主
        if (isLast && currentMaxBid == 0 && finalScore >= kBidLastOpenThreshold * thresholdMult) return 1;

        return 0;
    }
};
const vector<int> CardPatternAnalysis::reserve_value_table[18] = {
    {},//0
    {},//1
    {},//2
    {0, 1, 2, 3}, // 3
    {4, 5, 6, 7}, // 4
    {8, 9, 10, 11}, // 5
    {12, 13, 14, 15}, // 6
    {16, 17, 18, 19}, // 7
    {20, 21, 22, 23}, // 8
    {24, 25, 26, 27}, // 9
    {28, 29, 30, 31}, // 10
    {32, 33, 34, 35}, // J
    {36, 37, 38, 39}, // Q
    {40, 41, 42, 43}, // K
    {44, 45, 46, 47}, // A
    {48, 49, 50, 51}, // 2
    {52}, // 小王
    {53} // 大王
};

// 给一个候选动作打分：分数越高，越优先被搜索和兜底策略看到。
// 这个分数不是“绝对好坏”，只是用来决定先试哪些牌。
static int getActionSortScore(const vector<int>& action) {
    if (action.empty()) return -100;

    int type = CardPatternAnalysis::getCardTypeFromPoints(action);
    int size = (int)action.size();
    int mainValue = PatternCheck::getMainValueOfLastMove(action);

    int score = 0;

    // 长套优先：它们通常更利于快速走牌。
    switch (type) {
    case CardPatternAnalysis::TRIPLE_SEQUENCE_WITH_ONE:
    case CardPatternAnalysis::TRIPLE_SEQUENCE_WITH_TWO_PAIRS:
        score = 950 + size * 10;
        break;
    case CardPatternAnalysis::TRIPLE_SEQUENCE:
        score = 900 + size * 10;
        break;
    case CardPatternAnalysis::PAIR_SEQUENCE:
        score = 850 + size * 10;
        break;
    case CardPatternAnalysis::STRAIGHT:
        score = 800 + size * 10;
        break;
    case CardPatternAnalysis::QUAD_WITH_PAIRS:
    case CardPatternAnalysis::QUAD_WITH_SINGLES:
        score = 720 + size * 10;
        break;
    case CardPatternAnalysis::THREE_WITH_TWO:
    case CardPatternAnalysis::THREE_WITH_ONE:
        score = 650 + size * 10;
        break;
    case CardPatternAnalysis::TRIPLE:
        score = 500 + size * 10;
        break;
    case CardPatternAnalysis::PAIR:
        score = 300 + size * 10;
        break;
    case CardPatternAnalysis::SINGLE:
        score = 200 + size * 10;
        break;
    case CardPatternAnalysis::BOMB:
        score = 120 + mainValue;
        break;
    case CardPatternAnalysis::ROCKET:
        score = 100 + mainValue;
        break;
    default:
        score = 100 + size * 10;
        break;
    }

    // 同类型里，点数更小的动作一般更容易先试出“保留大牌”的路线。
    score += (17 - mainValue);
    return score;
}
// 手牌索引类。
// 它把“牌面点数 -> 实际牌号”整理好，方便快速取牌。
// 可以把它想成一个按点数分好的抽屉柜：要找某个点数的牌，直接去对应抽屉拿。
class HandIndex {
    array<vector<int>, 18> valueToCards;  // 点数 -> 牌号列表
    array<int, 18> valueCount;            // 点数 -> 数量
public:
    // 把一副手牌整理成“点数表”，方便后面快速找牌。
    // 输入是牌号列表，输出是内部索引表；它不改变原始手牌，只是把牌按点数存好。
    HandIndex(const vector<int>& hand) {
        valueCount.fill(0);
        for (int card : hand) {
            int v = CardPatternAnalysis::getCardValue(card);
            valueToCards[v].push_back(card);
            valueCount[v]++;
        }
    }

    // 看某个点数是否够指定张数。
    // 返回 true 表示可以取，false 表示这个点数的牌已经不够了。
    bool canTake(int value, int count) const {
        if (value < 3 || value > 17) return false; // 无效点数
        return valueCount[value] >= count;
    }

    // 取出某个点数的牌，返回真实牌号。
    // 这个函数会真的把牌从索引里删掉，所以它表示“拿走这些牌”。
    vector<int> takeCards(int value, int count) {
        if (!canTake(value, count)) return {};
        auto& vec = valueToCards[value];
        vector<int> res(vec.end() - count, vec.end());
        vec.erase(vec.end() - count, vec.end());
        valueCount[value] -= count;
        return res;
    }

    // 按一串点数去取牌，例如 [3,3,4]。
    // 适合把“点数序列”变成“真实能打出的牌号序列”。
    vector<int> takeCardsByValues(const vector<int>& values) {
        int need[18] = { 0 };
        for (int v : values) {
            if (v >= 3 && v <= 17) {
                need[v]++;
            }
        }
        for (int i = 3; i <= 17; ++i) {
            if (need[i] > valueCount[i] && need[i] > 0) return {};
        }
        vector<int> res;
        for (int v : values) {
            auto cards = takeCards(v, 1);
            if (cards.empty()) return {}; // 保守起见，虽然理论上不应该出现这种情况
            res.insert(res.end(), cards.begin(), cards.end());
        }
        return res;
    }

    // 把牌放回索引表里。
    // 搜索过程中经常会先试一手牌，再撤销，这个函数就是给“撤销”用的。
    void putCards(const vector<int>& cards) {
        for (int c : cards) {
            int v = CardPatternAnalysis::getCardValue(c);
            valueToCards[v].push_back(c);
            valueCount[v]++;
        }
    }

    // 统计现在还有多少张牌。
    // 只统计正常点数，不把非法值算进去。
    int totalCards() const {
        int total = 0;
        for (int i = 3; i <= 17; ++i) {
            total += valueCount[i];
        }
        return total;
    }
};

// 游戏状态。
// 这里保存当前谁出牌、谁是地主、还剩多少牌、历史出牌是什么。
// MCTS 每往下一层搜索，都会复制一份这个状态继续推演，所以它相当于“牌局快照”。
class GameState {
public:
    // 保存三个人各自还剩哪些点数的牌，使用 3 位一个点数的打包格式降低复制开销。
    uint64_t packedHand[3];
    int totalCards[3];
    vector<int> publiccard;// 底牌
    vector<int> lastMove;// 最近一次非过牌出牌
    int myRole; // 当前玩家角色 (0:地主, 1:农民甲, 2:农民乙)
    int landlordRole; // 地主角色索引（0, 1, 2），用于判断队友和对手
    int currentPassCount; // 当前连续过牌次数，用于判断是否需要重置跟牌压力
    int currentPlayer; // 当前玩家索引（0, 1, 2），用于轮流出牌
    int lastActionPlayer; // 上一个真正的出牌玩家索引（0, 1, 2），用于判断跟牌压力是否需要重置
    bool isGameOver; // 游戏是否结束
    int winner; // 0:地主胜, 1:农民甲胜, 2:农民乙胜
    mutable bool Actions_cached = false;// 是否已经缓存过当前状态的合法动作
    mutable vector<vector<int>> Cache_Actions;// 缓存当前状态的合法动作，避免重复计算
    mutable int currentCntBuffer[18] = {0};
    // 用已知信息创建一个新局面。
    // 输入包括三家手牌、底牌、历史出牌、地主是谁、我是谁。
    // 构造完成后，这个对象就能表示“当前局面走到哪里了”。
    GameState(const vector<int> hands_[3], const vector<int>& publiccards_, const vector<vector<int>>& history_, int landlord_role_, int my_role_)
        : publiccard(publiccards_),
        lastMove(),
        myRole(my_role_),
        landlordRole(landlord_role_),
        currentPassCount(0),
        currentPlayer(landlord_role_),
        lastActionPlayer(landlord_role_),
        isGameOver(false),
        winner(-1) {//需要优化初始化函数，暂且搁置
        // 1. 初始化手牌和底牌 (直接使用传入的参数)
        vector<int> numduy;
        for (int i = 0; i < 3; ++i) {
            int cnt[18] = {0};
            CardPatternAnalysis::analyzeHand(hands_[i], cnt, numduy);
            packedHand[i] = BitOps::packCounts(cnt);
            totalCards[i] = hands_[i].size();
        }
        
    }
    // 复制一个局面，方便搜索时继续往后试。
    // 搜索树会不断分叉，所以必须经常复制状态，不能直接在原局面上改。
    GameState(const GameState& other)
        :  publiccard(other.publiccard),      // 1
            lastMove(other.lastMove),          // 2
      myRole(other.myRole),              // 3
      landlordRole(other.landlordRole),  // 4
      currentPassCount(other.currentPassCount), // 5
      currentPlayer(other.currentPlayer),// 6
      lastActionPlayer(other.lastActionPlayer), // 7
      isGameOver(other.isGameOver),      // 8
      winner(other.winner),              // 9
      Actions_cached(false)  {
        for (int i = 0; i < 3; ++i) {
            packedHand[i] = other.packedHand[i];
            totalCards[i] = other.totalCards[i];
        }
    }
    GameState(GameState&& other) noexcept
        :
        publiccard(std::move(other.publiccard))
        , lastMove(std::move(other.lastMove))
        , myRole(other.myRole)
        , landlordRole(other.landlordRole)
        , currentPassCount(other.currentPassCount)
        , currentPlayer(other.currentPlayer)
        , lastActionPlayer(other.lastActionPlayer)
        , isGameOver(other.isGameOver)
        , winner(other.winner)
        , Actions_cached(false)      // 移动后缓存失效，安全
        , Cache_Actions()            // 清空缓存向量
    {
        for (int i = 0; i < 3; ++i) {
            packedHand[i] = other.packedHand[i];
            totalCards[i] = other.totalCards[i];
        }
    }
    GameState& operator=(GameState&& other) noexcept {
        if (this != &other) {
            publiccard = std::move(other.publiccard);
            lastMove = std::move(other.lastMove);
            myRole = other.myRole;
            landlordRole = other.landlordRole;
            currentPassCount = other.currentPassCount;
            currentPlayer = other.currentPlayer;
            lastActionPlayer = other.lastActionPlayer;
            isGameOver = other.isGameOver;
            winner = other.winner;
            Actions_cached = false;
            Cache_Actions.clear();
            for (int i = 0; i < 3; ++i) {
                packedHand[i] = other.packedHand[i];
                totalCards[i] = other.totalCards[i];
            }
        }
        return *this;
    }
    // 执行动作，并把局面改成下一回合。
    // action 为空表示过牌；不为空表示真正出牌，会扣掉手牌、记入历史、再切到下一位玩家。
    void applyActionInPlace(const vector<int>& action) {
        Actions_cached = false;
        Cache_Actions.clear();
        if (action.empty()) {// 1. 从玩家手牌中移除出牌
            this->currentPassCount++;// 过牌，增加连续过牌计数
            if (this->currentPassCount >= 2) {
                this->currentPlayer = this->lastActionPlayer;// 如果连续两人过牌，重置跟牌压力
                this->currentPassCount = 0;
            }
            else {
                this->currentPlayer = (this->currentPlayer + 1) % 3;
                 
            }
        }
        else {
            //出牌从当前的手牌中移除出牌
            for (int val : action) {
                this->packedHand[this->currentPlayer] = BitOps::addCount(this->packedHand[this->currentPlayer], val, -1);
                this->totalCards[this->currentPlayer]--;
            }
            //更新历史
            this->lastMove = action;
            //重置连续过牌计数，记录出牌者
            this->currentPassCount = 0;
            this->lastActionPlayer = this->currentPlayer;
            //下一家出牌
            this->currentPlayer = (this->currentPlayer + 1) % 3;
            if (this->totalCards[this->lastActionPlayer] == 0) {
                this->isGameOver = true;
                this->winner = this->lastActionPlayer;
            }
            
        }
    }
    // 复制一份局面，再在新局面里执行动作。
    // 这是一个“先模拟一步”的工具函数，不会影响原来的状态。
    GameState applyActionCopy(const vector<int>& action) const {
        GameState newState = *this;
        newState.applyActionInPlace(action);
        return newState;
    }
    // 取出底牌。
    // 这里只是读取，不会改动任何状态。
    vector<int> getPublicCard() const { return publiccard; }
    // 取出最近一次出牌。
    // 如果历史为空，说明当前还没有人真正出过牌。
    vector<int> getLastMove() const {
        return lastMove;
    }
    // 拿到当前轮到的玩家手牌表。
    // 返回的是“点数统计表”，后面的枚举函数会拿它来拼出所有可能动作。
    int* getCurrentPlayerHand() {
        BitOps::unpackCounts(this->packedHand[this->currentPlayer], this->currentCntBuffer);
        return this->currentCntBuffer;
    }
    // 看当前玩家是不是要“主动出牌”。
    // 主动出牌就是没有人压着你，你可以自由选择任意合法牌型。
    bool isLeading() const {
        if (lastMove.empty()) return true;
        else return lastActionPlayer == currentPlayer;
    }
    // 把当前玩家的点数分布复制到 cnt 里。
    // 这样做是为了让后面的牌型枚举函数统一使用同一种数据格式。
    void copyCurrentCnt(int cnt[18]) const {
        BitOps::unpackCounts(this->packedHand[currentPlayer], cnt);
    }

    void copyPlayerCnt(int player, int cnt[18]) const {
        BitOps::unpackCounts(this->packedHand[player], cnt);
    }

    void setPlayerCnt(int player, const int cnt[18]) {
        this->packedHand[player] = BitOps::packCounts(cnt);
        this->totalCards[player] = BitOps::totalCount(this->packedHand[player]);
        this->Actions_cached = false;
    }
    // 枚举当前局面下能出的所有合法牌。
    // 输入是当前牌局快照，输出是所有可以执行的动作集合。
    // 如果已经算过一次，就直接返回缓存结果，避免重复计算。
    vector<vector<int>> getAllActions() {
        if (Actions_cached) return Cache_Actions;
        int cnt[18];
        copyCurrentCnt(cnt);
        vector<int> lastMovePatterns = getLastMove();
        if (!lastMovePatterns.empty()) sort(lastMovePatterns.begin(), lastMovePatterns.end());
        int lastType = lastMovePatterns.empty() ? -1 : CardPatternAnalysis::getCardTypeFromPoints(lastMovePatterns);
        int lastMain = lastMovePatterns.empty() ? 0 : PatternCheck::getMainValueOfLastMove(lastMovePatterns);
        int lastLen = (lastMovePatterns.empty()) ? 0 : (int)lastMovePatterns.size();

        // 位掩码：每一位代表一个点数是否满足某个张数阈值。
        uint32_t singleMask = BitOps::makeMask(cnt, 1, 3, 17);
        uint32_t pairMask = BitOps::makeMask(cnt, 2, 3, 17);
        uint32_t tripleMask = BitOps::makeMask(cnt, 3, 3, 17);
        uint32_t bombMask = BitOps::makeMask(cnt, 4, 3, 15);

        vector<vector<int>> allActions;
        auto pushRepeat = [&](int rank, int repeat) {
            allActions.emplace_back(repeat, rank);
        };
        auto pushStraight = [&](int start, int len, int repeat) {
            vector<int> action;
            action.reserve(len * repeat);
            for (int i = 0; i < len; ++i) {
                for (int k = 0; k < repeat; ++k) action.push_back(start + i);
            }
            allActions.push_back(std::move(action));
        };
        auto pushLowestRanks = [&](uint32_t mask, int need, int repeat, int excludeRank = -1) {
            vector<int> chosen;
            while (mask && (int)chosen.size() < need) {
                int bit = __builtin_ctz(mask);
                int rank = 3 + bit;
                mask &= (mask - 1);
                if (rank == excludeRank) continue;
                chosen.push_back(rank);
            }
            if ((int)chosen.size() < need) return false;
            vector<int> action;
            action.reserve(need * repeat);
            for (int rank : chosen) {
                for (int i = 0; i < repeat; ++i) action.push_back(rank);
            }
            allActions.push_back(std::move(action));
            return true;
        };
        auto emitRuns = [&](uint32_t mask, int minLen, int repeat, int followLen, int followMain) {
            for (int bit = 0; bit <= 11; ) {
                if ((mask & (1u << bit)) == 0) { ++bit; continue; }
                int runStart = bit;
                while (bit <= 11 && (mask & (1u << bit))) ++bit;
                int runLen = bit - runStart;
                int wantMin = (followLen > 0) ? followLen : minLen;
                int wantMax = (followLen > 0) ? followLen : runLen;
                for (int len = wantMin; len <= wantMax && len <= runLen; ++len) {
                    if (followLen > 0 && len != followLen) continue;
                    for (int s = 0; s + len <= runLen; ++s) {
                        int start = 3 + runStart + s;
                        if (followLen > 0 && start <= followMain) continue;
                        pushStraight(start, len, repeat);
                    }
                }
            }
        };

        // 火箭
        if (cnt[16] >= 1 && cnt[17] >= 1) allActions.push_back({16, 17});

        if (isLeading()) {
            // 主动出牌：全部牌型枚举，顺序仍按大致优先级生成，最后统一排序。
            for (int bit = 0; bit <= 14; ++bit) if (singleMask & (1u << bit)) allActions.push_back({3 + bit});
            for (int bit = 0; bit <= 14; ++bit) if (pairMask & (1u << bit)) allActions.push_back({3 + bit, 3 + bit});
            for (int bit = 0; bit <= 14; ++bit) if (tripleMask & (1u << bit)) allActions.push_back({3 + bit, 3 + bit, 3 + bit});

            for (int bit = 0; bit <= 12; ++bit) {
                if ((bombMask & (1u << bit)) == 0) continue;
                allActions.push_back({3 + bit, 3 + bit, 3 + bit, 3 + bit});
            }

            emitRuns(BitOps::makeMask(cnt, 1, 3, 14), 5, 1, 0, 0);
            emitRuns(BitOps::makeMask(cnt, 2, 3, 14), 3, 2, 0, 0);
            emitRuns(BitOps::makeMask(cnt, 3, 3, 14), 2, 3, 0, 0);

            for (int bit = 0; bit <= 14; ++bit) {
                if ((tripleMask & (1u << bit)) == 0) continue;
                uint32_t attachSingles = singleMask & ~(1u << bit);
                vector<int> singles;
                for (uint32_t m = attachSingles; m && (int)singles.size() < 1; m &= (m - 1)) singles.push_back(3 + __builtin_ctz(m));
                if (!singles.empty()) allActions.push_back({3 + bit, 3 + bit, 3 + bit, singles[0]});

                uint32_t attachPairs = pairMask & ~(1u << bit);
                vector<int> pairs;
                for (uint32_t m = attachPairs; m && (int)pairs.size() < 1; m &= (m - 1)) pairs.push_back(3 + __builtin_ctz(m));
                if (!pairs.empty()) allActions.push_back({3 + bit, 3 + bit, 3 + bit, pairs[0], pairs[0]});
            }

            for (int bit = 0; bit <= 12; ++bit) {
                if ((bombMask & (1u << bit)) == 0) continue;
                uint32_t attachSingles = singleMask & ~(1u << bit);
                pushLowestRanks(attachSingles, 2, 1, 3 + bit);
                uint32_t attachPairs = pairMask & ~(1u << bit);
                vector<int> chosen;
                for (uint32_t m = attachPairs; m && (int)chosen.size() < 2; m &= (m - 1)) chosen.push_back(3 + __builtin_ctz(m));
                if ((int)chosen.size() == 2) {
                    allActions.push_back({3 + bit, 3 + bit, 3 + bit, 3 + bit, chosen[0], chosen[0], chosen[1], chosen[1]});
                }
            }

            for (int len = 2; len <= 4; ++len) {
                uint32_t runMask = tripleMask & ((1u << 12) - 1u);
                emitRuns(runMask, len, 3, 0, 0);
                break;
            }

            for (int start = 3; start <= 14; ++start) {
                for (int len = 2; len <= 4 && start + len - 1 <= 14; ++len) {
                    uint32_t bodyMask = ((1u << len) - 1u) << (start - 3);
                    if ((tripleMask & bodyMask) != bodyMask) continue;
                    uint32_t attachSingles = singleMask & ~bodyMask;
                    vector<int> singles;
                    for (uint32_t m = attachSingles; m && (int)singles.size() < len; m &= (m - 1)) singles.push_back(3 + __builtin_ctz(m));
                    if ((int)singles.size() < len) continue;
                    vector<int> action;
                    action.reserve(len * 4);
                    pushStraight(start, len, 3);
                    action = allActions.back();
                    action.insert(action.end(), singles.begin(), singles.begin() + len);
                    allActions.back() = std::move(action);
                }
            }

            for (int start = 3; start <= 14; ++start) {
                for (int len = 2; len <= 4 && start + len - 1 <= 14; ++len) {
                    uint32_t bodyMask = ((1u << len) - 1u) << (start - 3);
                    if ((tripleMask & bodyMask) != bodyMask) continue;
                    uint32_t attachPairs = pairMask & ~bodyMask;
                    vector<int> pairs;
                    for (uint32_t m = attachPairs; m && (int)pairs.size() < len; m &= (m - 1)) pairs.push_back(3 + __builtin_ctz(m));
                    if ((int)pairs.size() < len) continue;
                    vector<int> action;
                    action.reserve(len * 5);
                    for (int i = 0; i < len; ++i) for (int k = 0; k < 3; ++k) action.push_back(start + i);
                    for (int i = 0; i < len; ++i) { action.push_back(pairs[i]); action.push_back(pairs[i]); }
                    allActions.push_back(std::move(action));
                }
            }
        }
        else {
            // 跟牌：先加炸弹/火箭（可跨类型）
            if (!lastMovePatterns.empty() && lastType == CardPatternAnalysis::ROCKET) {
                Cache_Actions = { {} };
                Actions_cached = true;
                return { {} };
            }

            if (lastType == CardPatternAnalysis::BOMB) {
                for (int bit = 0; bit <= 12; ++bit) {
                    if ((bombMask & (1u << bit)) == 0) continue;
                    int rank = 3 + bit;
                    if (rank <= lastMain) continue;
                    allActions.push_back({rank, rank, rank, rank});
                }
            }
            else {
                for (int bit = 0; bit <= 12; ++bit) {
                    if (bombMask & (1u << bit)) {
                        int rank = 3 + bit;
                        allActions.push_back({rank, rank, rank, rank});
                    }
                }
            }

            switch (lastType) {
            case CardPatternAnalysis::SINGLE: {
                uint32_t follow = BitOps::makeMask(cnt, 1, 3, 17);
                for (uint32_t m = follow & ~((1u << (lastMain - 3 + 1)) - 1u); m; m &= (m - 1)) {
                    int rank = 3 + __builtin_ctz(m);
                    if (rank > lastMain) allActions.push_back({rank});
                }
                break;
            }
            case CardPatternAnalysis::PAIR: {
                uint32_t follow = BitOps::makeMask(cnt, 2, 3, 17);
                for (uint32_t m = follow & ~((1u << (lastMain - 3 + 1)) - 1u); m; m &= (m - 1)) {
                    int rank = 3 + __builtin_ctz(m);
                    if (rank > lastMain) allActions.push_back({rank, rank});
                }
                break;
            }
            case CardPatternAnalysis::TRIPLE: {
                uint32_t follow = BitOps::makeMask(cnt, 3, 3, 17);
                for (uint32_t m = follow & ~((1u << (lastMain - 3 + 1)) - 1u); m; m &= (m - 1)) {
                    int rank = 3 + __builtin_ctz(m);
                    if (rank > lastMain) allActions.push_back({rank, rank, rank});
                }
                break;
            }
            case CardPatternAnalysis::STRAIGHT:
                emitRuns(BitOps::makeMask(cnt, 1, 3, 14), 5, 1, lastLen, lastMain);
                break;
            case CardPatternAnalysis::PAIR_SEQUENCE:
                emitRuns(BitOps::makeMask(cnt, 2, 3, 14), 3, 2, lastLen / 2, lastMain);
                break;
            case CardPatternAnalysis::TRIPLE_SEQUENCE:
                emitRuns(BitOps::makeMask(cnt, 3, 3, 14), 2, 3, lastLen / 3, lastMain);
                break;
            case CardPatternAnalysis::THREE_WITH_ONE:
                for (int bit = 0; bit <= 14; ++bit) {
                    if ((tripleMask & (1u << bit)) == 0) continue;
                    int rank = 3 + bit;
                    if (rank <= lastMain) continue;
                    uint32_t attachSingles = singleMask & ~(1u << bit);
                    if (attachSingles) {
                        int attach = 3 + __builtin_ctz(attachSingles);
                        allActions.push_back({rank, rank, rank, attach});
                    }
                }
                break;
            case CardPatternAnalysis::THREE_WITH_TWO:
                for (int bit = 0; bit <= 14; ++bit) {
                    if ((tripleMask & (1u << bit)) == 0) continue;
                    int rank = 3 + bit;
                    if (rank <= lastMain) continue;
                    uint32_t attachPairs = pairMask & ~(1u << bit);
                    if (attachPairs) {
                        int attach = 3 + __builtin_ctz(attachPairs);
                        allActions.push_back({rank, rank, rank, attach, attach});
                    }
                }
                break;
            case CardPatternAnalysis::QUAD_WITH_SINGLES:
                for (int bit = 0; bit <= 12; ++bit) {
                    if ((bombMask & (1u << bit)) == 0) continue;
                    int rank = 3 + bit;
                    if (rank <= lastMain) continue;
                    uint32_t attachSingles = singleMask & ~(1u << bit);
                    vector<int> chosen;
                    for (uint32_t m = attachSingles; m && (int)chosen.size() < 2; m &= (m - 1)) chosen.push_back(3 + __builtin_ctz(m));
                    if ((int)chosen.size() == 2) allActions.push_back({rank, rank, rank, rank, chosen[0], chosen[1]});
                }
                break;
            case CardPatternAnalysis::QUAD_WITH_PAIRS:
                for (int bit = 0; bit <= 12; ++bit) {
                    if ((bombMask & (1u << bit)) == 0) continue;
                    int rank = 3 + bit;
                    if (rank <= lastMain) continue;
                    uint32_t attachPairs = pairMask & ~(1u << bit);
                    vector<int> chosen;
                    for (uint32_t m = attachPairs; m && (int)chosen.size() < 2; m &= (m - 1)) chosen.push_back(3 + __builtin_ctz(m));
                    if ((int)chosen.size() == 2) allActions.push_back({rank, rank, rank, rank, chosen[0], chosen[0], chosen[1], chosen[1]});
                }
                break;
            case CardPatternAnalysis::TRIPLE_SEQUENCE_WITH_ONE:
                emitRuns(BitOps::makeMask(cnt, 3, 3, 14), 2, 3, lastLen / 4, lastMain);
                break;
            case CardPatternAnalysis::TRIPLE_SEQUENCE_WITH_TWO_PAIRS:
                emitRuns(BitOps::makeMask(cnt, 3, 3, 14), 2, 3, lastLen / 5, lastMain);
                break;
            default: break;
            }

            allActions.push_back({}); // 过牌
        }

        sort(allActions.begin(), allActions.end(), [](const vector<int>& a, const vector<int>& b) {
            int scoreA = getActionSortScore(a);
            int scoreB = getActionSortScore(b);
            if (scoreA != scoreB) return scoreA > scoreB;
            if (a.size() != b.size()) return a.size() > b.size();
            return a < b;
        });

        Cache_Actions = allActions;
        Actions_cached = true;
        return allActions;
    }
    // 牌很少的时候，直接用精确搜索判断输赢。
    // 它不是随机模拟，而是把所有分支尽量都算一遍，适合终局附近做“精算”。
    static int exactsearch(GameState& state, int depth, int myId, int alpha, int beta) {
        if (state.isGameOver) {
            bool landlordWin = (state.winner == state.landlordRole);
            bool iAmLandlord = (myId == state.landlordRole);
            return (landlordWin == iAmLandlord) ? 1 : -1;
        }
        if (depth <= 0)return 0;
        auto actions = state.getAllActions();
        sort(actions.begin(), actions.end(),
            [](const vector<int>& a, const vector<int>& b) {
                return a.size() < b.size();
            });
        if (actions.empty())actions.push_back({});
        // 判断当前玩家是否属于我方（地主独立，农民二人同盟）
        auto isMyTeam = [&](int player) -> bool {
            if (myId == state.landlordRole) {
                return player == myId;               // 我是地主，只有我是我方
            } else {
                return player != state.landlordRole; // 我是农民，所有农民都是我方
            }
        };
        if (isMyTeam(state.currentPlayer)) {
            int value = -2;
            for (const auto& act : actions) {
                GameState next = state.applyActionCopy(act);
                int child = exactsearch(next, depth - 1, myId, alpha, beta);
                if (child > value)value = child;
                if (value > alpha)alpha = value;
                if (alpha >= beta)break;
            }
            return value;
        }
        else {
            int value = 2;
            for (const auto& act : actions) {
                GameState next = state.applyActionCopy(act);
                int child = exactsearch(next, depth - 1, myId, alpha, beta);
                if (child < value)value = child;
                if (value < beta)beta = value;
                if (alpha >= beta)break;
            }
            return value;
        }
    }
};
//====按需生成器====//
// 这是一个“简化版出牌建议器”。
// 它不负责全局最优，只负责在模拟阶段或兜底阶段快速给出一个还不错的动作。
std::vector<int> getBestActionByPriority(
    const int* hand,
    const std::vector<int>& lastMove,
    int currentPlayer,
    int landlordRole,
    int lastActionPlayer,
    int myCardsLeft,
    int minEnemyCards,
    int teammateCards = -1
)
{
    // 复制手牌统计
    int cnt[18];
    std::memcpy(cnt, hand, 18 * sizeof(int));

    std::vector<int> lastMovePatterns;
    bool isLeading = lastMove.empty();
    if (!isLeading) {
        lastMovePatterns = lastMove;
        std::sort(lastMovePatterns.begin(), lastMovePatterns.end());
    }

    uint32_t singleMask = BitOps::makeMask(cnt, 1, 3, 17);
    uint32_t pairMask = BitOps::makeMask(cnt, 2, 3, 17);
    uint32_t tripleMask = BitOps::makeMask(cnt, 3, 3, 17);
    uint32_t bombMask = BitOps::makeMask(cnt, 4, 3, 15);

    auto pickLowestRank = [&](uint32_t mask, int excludeRank = -1) -> int {
        if (excludeRank >= 3 && excludeRank <= 17) mask &= ~(1u << (excludeRank - 3));
        if (!mask) return -1;
        return 3 + __builtin_ctz(mask);
    };
    auto buildStraight = [&](int start, int len, int repeat) {
        std::vector<int> action;
        action.reserve(len * repeat);
        for (int i = 0; i < len; ++i) for (int k = 0; k < repeat; ++k) action.push_back(start + i);
        return action;
    };

    //送牌
    bool isFarmer = (currentPlayer != landlordRole);
    if (isFarmer && teammateCards > 0 && teammateCards <= 2) {
        // 找最小的单张，没有则找最小的对子，再没有则找最小的三张
        int v = pickLowestRank(singleMask);
        if (v != -1) return {v};
        v = pickLowestRank(pairMask);
        if (v != -1) return {v, v};
        v = pickLowestRank(tripleMask);
        if (v != -1) return {v, v, v};
    }
    // ----- 1. 直接获胜处理 -----
    if (myCardsLeft <= 2 && isLeading) {
        int v = pickLowestRank(singleMask);
        if (v != -1) return {v};
        v = pickLowestRank(pairMask);
        if (v != -1) return {v, v};
    }

    // ----- 2. 火箭/炸弹可用性判断 -----
    
    bool canUseBomb = true;
    if (!isLeading && isFarmer) {
        // 农民跟牌时，只有上家是地主时才允许出炸弹/火箭，除非情况紧急
        if (lastActionPlayer != landlordRole) {
            // 上家是队友
            bool urgent = (minEnemyCards <= 2); // 对手马上要赢，可以不管队友直接炸
            canUseBomb = urgent;
        } else {
            canUseBomb = true; // 上家是地主，可以炸
        }
    }
    if (!isLeading && lastMovePatterns.size() == 2 && lastMovePatterns[0] == 16 && lastMovePatterns[1] == 17)
        canUseBomb = false; // 上家出火箭，炸弹无意义

    int lastType = isLeading ? -1 : CardPatternAnalysis::getCardTypeFromPoints(lastMovePatterns);
    int lastMain = isLeading ? 0 : PatternCheck::getMainValueOfLastMove(lastMovePatterns);

    // 位运算快速路径：先处理最常见的同类型/顺子跟牌。
    if (!isLeading) {
        if (lastType == CardPatternAnalysis::SINGLE) {
            int v = pickLowestRank(singleMask, lastMain);
            if (v != -1 && v > lastMain) return {v};
        } else if (lastType == CardPatternAnalysis::PAIR) {
            int v = pickLowestRank(pairMask, lastMain);
            if (v != -1 && v > lastMain) return {v, v};
        } else if (lastType == CardPatternAnalysis::TRIPLE) {
            int v = pickLowestRank(tripleMask, lastMain);
            if (v != -1 && v > lastMain) return {v, v, v};
        } else if (lastType == CardPatternAnalysis::STRAIGHT) {
            int needLen = (int)lastMovePatterns.size();
            uint32_t runMask = BitOps::makeMask(cnt, 1, 3, 14);
            for (int bit = 0; bit <= 11; ) {
                if ((runMask & (1u << bit)) == 0) { ++bit; continue; }
                int runStart = bit;
                while (bit <= 11 && (runMask & (1u << bit))) ++bit;
                int runLen = bit - runStart;
                for (int s = 0; s + needLen <= runLen; ++s) {
                    int start = 3 + runStart + s;
                    if (start > lastMain) return buildStraight(start, needLen, 1);
                }
            }
        } else if (lastType == CardPatternAnalysis::PAIR_SEQUENCE) {
            int needLen = (int)lastMovePatterns.size() / 2;
            uint32_t runMask = BitOps::makeMask(cnt, 2, 3, 14);
            for (int bit = 0; bit <= 11; ) {
                if ((runMask & (1u << bit)) == 0) { ++bit; continue; }
                int runStart = bit;
                while (bit <= 11 && (runMask & (1u << bit))) ++bit;
                int runLen = bit - runStart;
                for (int s = 0; s + needLen <= runLen; ++s) {
                    int start = 3 + runStart + s;
                    if (start > lastMain) return buildStraight(start, needLen, 2);
                }
            }
        } else if (lastType == CardPatternAnalysis::TRIPLE_SEQUENCE) {
            int needLen = (int)lastMovePatterns.size() / 3;
            uint32_t runMask = BitOps::makeMask(cnt, 3, 3, 14);
            for (int bit = 0; bit <= 11; ) {
                if ((runMask & (1u << bit)) == 0) { ++bit; continue; }
                int runStart = bit;
                while (bit <= 11 && (runMask & (1u << bit))) ++bit;
                int runLen = bit - runStart;
                for (int s = 0; s + needLen <= runLen; ++s) {
                    int start = 3 + runStart + s;
                    if (start > lastMain) return buildStraight(start, needLen, 3);
                }
            }
        }
    }

    // ----- 3. 生成候选动作（直接顺序检查，避免每次调用分配闭包容器）-----
    auto toAction = [&](auto&& p) -> std::vector<int> {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, Rocket>) {
            return {16, 17};
        } else if constexpr (std::is_same_v<T, Bomb>) {
            return {p.value, p.value, p.value, p.value};
        } else if constexpr (std::is_same_v<T, Single>) {
            return {p.value};
        } else if constexpr (std::is_same_v<T, Pair>) {
            return {p.value, p.value};
        } else if constexpr (std::is_same_v<T, Triple>) {
            return {p.value, p.value, p.value};
        } else if constexpr (std::is_same_v<T, Straight>) {
            std::vector<int> v;
            for (int i = 0; i < p.len; ++i) v.push_back(p.start + i);
            return v;
        } else if constexpr (std::is_same_v<T, PairSequence>) {
            std::vector<int> v;
            for (int i = 0; i < p.len; ++i) { v.push_back(p.start + i); v.push_back(p.start + i); }
            return v;
        } else if constexpr (std::is_same_v<T, TripleSequence>) {
            std::vector<int> v;
            for (int i = 0; i < p.len; ++i) { v.push_back(p.start + i); v.push_back(p.start + i); v.push_back(p.start + i); }
            return v;
        } else if constexpr (std::is_same_v<T, TripleWithOne>) {
            return {p.triple, p.triple, p.triple, p.single};
        } else if constexpr (std::is_same_v<T, TripleWithTwo>) {
            return {p.triple, p.triple, p.triple, p.pair, p.pair};
        } else if constexpr (std::is_same_v<T, QuadWithSingles>) {
            return {p.quad, p.quad, p.quad, p.quad, p.single1, p.single2};
        } else if constexpr (std::is_same_v<T, QuadWithPairs>) {
            return {p.quad, p.quad, p.quad, p.quad, p.pair1, p.pair1, p.pair2, p.pair2};
        } else if constexpr (std::is_same_v<T, PlanWithSingles>) {
            std::vector<int> v;
            for (int i = 0; i < p.len; ++i) { v.push_back(p.start + i); v.push_back(p.start + i); v.push_back(p.start + i); }
            for (int s : p.singles) v.push_back(s);
            return v;
        } else if constexpr (std::is_same_v<T, PlanWithPairs>) {
            std::vector<int> v;
            for (int i = 0; i < p.len; ++i) { v.push_back(p.start + i); v.push_back(p.start + i); v.push_back(p.start + i); }
            for (int pr : p.pairs) { v.push_back(pr); v.push_back(pr); }
            return v;
        }
        return std::vector<int>{};
    };

    auto tryPattern = [&](auto&& pattern) -> std::vector<int> {
        using T = std::decay_t<decltype(pattern)>;
        T matched;
        if (PatternCheck::check(cnt, lastMovePatterns, matched)) {
            return toAction(matched);
        }
        return {};
    };

    if (isLeading || canUseBomb) {
        Bomb bomb;
        if (PatternCheck::check(cnt, lastMovePatterns, bomb)) return toAction(bomb);
        Rocket rocket;
        if (PatternCheck::check(cnt, lastMovePatterns, rocket)) return toAction(rocket);
    }
    if (Straight straight; PatternCheck::check(cnt, lastMovePatterns, straight)) return toAction(straight);
    if (PairSequence pairSequence; PatternCheck::check(cnt, lastMovePatterns, pairSequence)) return toAction(pairSequence);
    if (TripleSequence tripleSequence; PatternCheck::check(cnt, lastMovePatterns, tripleSequence)) return toAction(tripleSequence);
    if (PlanWithSingles planWithSingles; PatternCheck::check(cnt, lastMovePatterns, planWithSingles)) return toAction(planWithSingles);
    if (PlanWithPairs planWithPairs; PatternCheck::check(cnt, lastMovePatterns, planWithPairs)) return toAction(planWithPairs);
    if (TripleWithOne tripleWithOne; PatternCheck::check(cnt, lastMovePatterns, tripleWithOne)) return toAction(tripleWithOne);
    if (TripleWithTwo tripleWithTwo; PatternCheck::check(cnt, lastMovePatterns, tripleWithTwo)) return toAction(tripleWithTwo);
    if (QuadWithSingles quadWithSingles; PatternCheck::check(cnt, lastMovePatterns, quadWithSingles)) return toAction(quadWithSingles);
    if (QuadWithPairs quadWithPairs; PatternCheck::check(cnt, lastMovePatterns, quadWithPairs)) return toAction(quadWithPairs);
    if (Triple triple; PatternCheck::check(cnt, lastMovePatterns, triple)) return toAction(triple);
    if (Pair pair; PatternCheck::check(cnt, lastMovePatterns, pair)) return toAction(pair);
    if (Single single; PatternCheck::check(cnt, lastMovePatterns, single)) return toAction(single);

    // 没有合法动作，过牌
    return {};
}
// ========== 粒子滤波类 ==========
struct OpponentParticle {
    int hand1[18] = {0};
    int hand2[18] = {0};
};

class ParticleFilter {
public:
    static constexpr int k = 100;                // 粒子数量，降低主线程初始化和更新开销
    static int myRole;
    static int opp1Id, opp2Id;

    // 新增：需要由 main 设置的上下文信息
    static int finalBid;                        // 最终叫分 (1~3)
    static std::vector<int> bidHistory;         // 各玩家叫分序列
    static std::vector<int> ownInitialHand;     // 己方初始手牌（点数，地主含底牌）
    
    // 动态维护的状态
    static std::vector<int> livePool;           // 当前未知牌点数池
    static int remainOpp1, remainOpp2;          // 两个对手当前剩余牌数
    static std::vector<OpponentParticle> particles;

    static int targetParticleCount() {
        int unknown = remainOpp1 + remainOpp2;
        if (unknown <= 10) return 24;
        if (unknown <= 16) return 36;
        if (unknown <= 24) return 50;
        if (unknown <= 34) return 72;
        return k;
    }

    // 初始化粒子（不扣除任何历史）
    static void initialize(const GameState& rootState) {
        myRole = rootState.myRole;
        int landlord = rootState.landlordRole;
        opp1Id = (myRole + 1) % 3;
        opp2Id = (myRole + 2) % 3;

        // 1. 构建初始未知牌池（54张牌点数）
        livePool.clear();
        for (int v = 3; v <= 15; ++v) livePool.insert(livePool.end(), 4, v);
        livePool.push_back(16);  // 小王
        livePool.push_back(17);  // 大王

        // 移除自己的初始手牌
        for (int v : ownInitialHand) {
            auto it = std::find(livePool.begin(), livePool.end(), v);
            if (it != livePool.end()) livePool.erase(it);
        }

        // 如果我是农民，底牌已在地主手中，未知池不应再包含底牌点数
        if (myRole != landlord) {
            for (int card : rootState.getPublicCard()) {
                int v = CardPatternAnalysis::getCardValue(card);
                auto it = std::find(livePool.begin(), livePool.end(), v);
                if (it != livePool.end()) livePool.erase(it);
            }
        }

        // 2. 初始对手剩余牌数
        remainOpp1 = (opp1Id == landlord) ? 20 : 17;
        remainOpp2 = (opp2Id == landlord) ? 20 : 17;
        int sampleOpp1 = remainOpp1;
        int sampleOpp2 = remainOpp2;
        if (myRole != landlord) {
            if (opp1Id == landlord) sampleOpp1 -= 3;
            if (opp2Id == landlord) sampleOpp2 -= 3;
        }

        // 3. 权重函数（根据叫分调整大牌概率）
        auto getWeight = [&](int value, int player) -> double {
            double w = 1.0;
            if (player == landlord) {
                if (finalBid >= 3) {
                    if (value >= 14) w = 3.0;
                    else if (value >= 12) w = 2.0;
                } else if (finalBid >= 2) {
                    if (value >= 15) w = 2.5;
                    else if (value >= 13) w = 1.5;
                } else if (finalBid == 1) {
                    if (value >= 15) w = 1.5;
                }
            }
            return w;
        };

        // 4. 生成较少的临时粒子，残局时自动降配
        int INIT_NUM = std::max(targetParticleCount(), 40);
        std::vector<OpponentParticle> tmp(INIT_NUM);
        for (int i = 0; i < INIT_NUM; ++i) {
            OpponentParticle& p = tmp[i];
            std::vector<int> poolCopy = livePool;

            // 分配 opp1
            for (int j = 0; j < sampleOpp1; ++j) {
                std::vector<double> weights(poolCopy.size());
                for (size_t idx = 0; idx < poolCopy.size(); ++idx)
                    weights[idx] = getWeight(poolCopy[idx], opp1Id);
                std::discrete_distribution<int> dist(weights.begin(), weights.end());
                int idx = dist(globalRng);
                int card = poolCopy[idx];
                p.hand1[card]++;
                poolCopy.erase(poolCopy.begin() + idx);
            }
            // 分配 opp2
            for (int j = 0; j < sampleOpp2; ++j) {
                std::vector<double> weights(poolCopy.size());
                for (size_t idx = 0; idx < poolCopy.size(); ++idx)
                    weights[idx] = getWeight(poolCopy[idx], opp2Id);
                std::discrete_distribution<int> dist(weights.begin(), weights.end());
                int idx = dist(globalRng);
                int card = poolCopy[idx];
                p.hand2[card]++;
                poolCopy.erase(poolCopy.begin() + idx);
            }
            if (myRole != landlord) {
                for (int card : rootState.getPublicCard()) {
                    int v = CardPatternAnalysis::getCardValue(card);
                    if (opp1Id == landlord) p.hand1[v]++;
                    else p.hand2[v]++;
                }
            }
        }

        // 5. 随机保留 k 个粒子
        std::shuffle(tmp.begin(), tmp.end(), globalRng);
        particles.clear();
        int targetK = targetParticleCount();
        targetK = std::min(targetK, (int)tmp.size());
        particles.assign(tmp.begin(), tmp.begin() + targetK);
    }

    // 增量更新：对手出了一手牌
    static void update(const std::vector<int>& action, int opponentId) {
        if (action.empty()) return;

        // 1. 统计出牌点数
        int playedCnt[18] = {0};
        for (int v : action) playedCnt[v]++;

        // 2. 筛选并扣牌
        std::vector<OpponentParticle> survivors;
        for (const auto& p : particles) {
            const int* hand = (opponentId == opp1Id) ? p.hand1 : p.hand2;
            bool valid = true;
            for (int v = 3; v <= 17; ++v) {
                if (hand[v] < playedCnt[v]) { valid = false; break; }
            }
            if (!valid) continue;

            OpponentParticle updated = p;
            int* updHand = (opponentId == opp1Id) ? updated.hand1 : updated.hand2;
            for (int v = 3; v <= 17; ++v) updHand[v] -= playedCnt[v];
            survivors.push_back(updated);
        }

        // 3. 更新对手剩余牌数
        if (opponentId == opp1Id) remainOpp1 -= (int)action.size();
        else                      remainOpp2 -= (int)action.size();

        // 4. 从全局未知牌池移除这些牌
        for (int v : action) {
            auto it = std::find(livePool.begin(), livePool.end(), v);
            if (it != livePool.end()) livePool.erase(it);
        }

        // 5. 重采样
        int targetK = targetParticleCount();
        if (survivors.empty() || (int)survivors.size() < std::max(4, targetK / 5)) {
            // 粒子退化严重，用当前 livePool 和剩余牌数重新生成动态数量的粒子
            particles.clear();
            int need1 = remainOpp1, need2 = remainOpp2;
            while ((int)livePool.size() < need1 + need2)
                livePool.push_back(3);  // 保险

            for (int i = 0; i < targetK; ++i) {
                OpponentParticle p{};
                std::vector<int> poolCopy = livePool;
                std::shuffle(poolCopy.begin(), poolCopy.end(), globalRng);
                int idx = 0;
                for (int j = 0; j < need1 && idx < (int)poolCopy.size(); ++j)
                    p.hand1[poolCopy[idx++]]++;
                for (int j = 0; j < need2 && idx < (int)poolCopy.size(); ++j)
                    p.hand2[poolCopy[idx++]]++;
                particles.push_back(p);
            }
        } else {
            // 从幸存者中均匀重采样动态数量的粒子
            std::uniform_int_distribution<int> dist(0, survivors.size() - 1);
            particles.clear();
            for (int i = 0; i < targetK; ++i)
                particles.push_back(survivors[dist(globalRng)]);
        }
    }

    // 获取一个随机粒子
    static OpponentParticle sample() {
        if (particles.empty()) return OpponentParticle{};
        int idx = randomInt(0, (int)particles.size() - 1);
        return particles[idx];
    }
};

 
// 蒙特卡洛树搜索（MCTS）。
// 简单理解就是：先试几个动作，再把局面往后模拟，看哪个动作更容易赢。
// 选择：用“历史表现 + 探索奖励 + 先验分数”挑最值得继续看的分支。
// 扩展：给当前节点加一个新的孩子节点，表示尝试一种新动作。
// 模拟：从这个新局面开始，用简化规则一直走到结束，看看最后是赢还是输。
// 回传：把这次模拟结果一路传回父节点，更新每个节点的访问次数和胜率。
class MCTSNode {
public:
    GameState* state;
    MCTSNode* parent;
    vector<MCTSNode*> children;
    vector<int> action; // 从父节点到当前节点的动作
    vector<vector<int>> actions;   // 当前节点的所有合法动作（点数序列）
    int visits;//历史场数
    int wins;//胜利场数
    int prior; // 权重
    int max_score; // 用于归一化先验概率的动态常数
    // 先验概率，可以根据启发式评估函数计算得到,evaluateHand函数可以用来评估当前手牌的好坏程度，作为先验概率的一部分
    //先验概率的归一化可以通过将评估分数除以一个动态常数即每个子动作的评估分数中最大的来实现，使得先验概率在0到1之间。
    // 构造一个搜索节点：保存当前局面、动作和先验分数。
    // 输入是一个 GameState，以及它在搜索树里的父节点和动作。
    // 构造完以后，这个节点就代表“在当前局面下，走了这个动作之后会怎样”。
    MCTSNode(GameState& otherstate, MCTSNode* parent = nullptr, const vector<int>& action = {},int ext_max_score = -1)
        : parent(parent), visits(0), wins(0), max_score(0) {
        this->state = new GameState(otherstate);
        this->action = action;
        if (parent == nullptr) {
            actions = state->getAllActions();           // 根节点全量枚举
        }
        else {
            actions = deriveChildActions(parent->actions, action);  // 子节点增量派生
        }
        // 计算先验分数（未归一化）
        int priorCnt[18];
        state->copyPlayerCnt(state->myRole, priorCnt);
        int raw_prior = CardPatternAnalysis::evaluateHand(priorCnt);
       if (ext_max_score >= 0) {
        max_score = ext_max_score;                 // 使用外部统一最大值
        } else if (parent) {
            max_score = max(parent->max_score, raw_prior);
        } else {
            max_score = raw_prior;
        }
    prior = (max_score > 0) ? (raw_prior * 100) / max_score : 0;
    }

    // 节点销毁时，把子节点一起释放。
    // 这样可以避免搜索树结束后还残留一堆没释放的内存。
    ~MCTSNode() {
        delete state;
        for (MCTSNode* child : children) {
            delete child;
        }
    }

    // 从孩子节点里挑一个最值得继续往下搜的。
    // 它会同时看“赢得多不多”“试得够不够多”“先验分数高不高”。
    MCTSNode* selectChild() {
        MCTSNode* bestChild = nullptr;
        double bestScore = -1e9;
        int totalRemaining = state->totalCards[0] + state->totalCards[1] + state->totalCards[2];
        double C = 2.8 + 0.3 * (double)totalRemaining / 54.0;
        for (MCTSNode* child : children) {
            double exploit = (child->visits > 0) ? (double)child->wins / child->visits : 0.0;
            double explore = C * sqrt(log(visits + 1) / (child->visits + 1e-6));
            double priorWeight = 0.3 / (1.0 + visits / 25.0);
            double UCB = exploit + explore + priorWeight * child->prior / 100.0;
            if (UCB > bestScore) {
                bestScore = UCB;
                bestChild = child;
            }
        }
        return bestChild;
    }

    // 从当前节点往下多加一个新孩子。
    // 这个步骤就是“扩展”：先把还没试过的动作拿出来，再选一个最有希望的动作建成子节点。
    void expand() {
       //剔除已经扩展过的动作
        vector<vector<int>> untried;
        for (const auto& action : actions) {
            bool tried = false;
            for (MCTSNode* child : children) {
                if (child->action == action) {
                    tried = true;
                    break;
                }
            }
            if (!tried) untried.push_back(action);
        }
        if (untried.empty()) return; // 没有未尝试的动作了,隐藏问题：没有合法动作时调用方不知
         // 计算所有未尝试动作的评分
        vector<pair<int, vector<int>>> scored;  // (score, action)
        int max_score = 0;
        for (const auto& act : untried) {
            GameState temp = state->applyActionCopy(act);
            int tempCnt[18];
            temp.copyPlayerCnt(temp.myRole, tempCnt);
            int s = CardPatternAnalysis::evaluateHand(tempCnt);
            scored.emplace_back(s, act);
            if (s > max_score) max_score = s;
        }
        if (max_score == 0) max_score = 1;
        // 按分数降序排序
        sort(scored.begin(), scored.end(), [](auto& a, auto& b) { return a.first > b.first; });

        // 一次扩展最多 2 个子节点（可根据剩余时间动态调整）
        const int MAX_EXPAND = 2;
        for (int i = 0; i < MAX_EXPAND && i < (int)scored.size(); ++i) {
            GameState newState = state->applyActionCopy(scored[i].second);
            // 为该子节点创建时传入外部 max_score 以保持归一化一致
            MCTSNode* child = new MCTSNode(newState, this, scored[i].second, max_score);
            children.push_back(child);
        }

    }


    // 用简化规则把牌局往后走到结束，看看结果好不好。
    // 这是 MCTS 里的“模拟”阶段：它不追求绝对准确，只追求快，而且能大致反映这个动作值不值得。
    bool simulate() {
        GameState simState = *state;
        if (isUseExactSearch(simState)) {
            int maxDepth = simState.totalCards[simState.currentPlayer];
            int result = GameState::exactsearch(simState, maxDepth, state->myRole, -2, 2);
            return result == 1;
        }
        const double EPSILON = 0.26;
        uniform_real_distribution<double> dist(0.0, 1.0);
        const int MAX_SIM_STEPS = 143;
        int stepCount = 0;
        while (!simState.isGameOver && stepCount < MAX_SIM_STEPS) {
            stepCount++;
            vector<int> action;
            if (dist(globalRng)< EPSILON) {
                // 以一定概率选择一个随机合法动作，增加探索
                vector<vector<int>> legalActions = simState.getAllActions();
                if (!legalActions.empty()) {
                    int randomIdx = randomInt(0, legalActions.size() - 1);
                    simState.applyActionInPlace(legalActions[randomIdx]);
                }
            }
            else {
                int currentPlayer = simState.currentPlayer;
                int myCards = simState.totalCards[currentPlayer];
                int enemyCards = std::min(simState.totalCards[(currentPlayer+1)%3], simState.totalCards[(currentPlayer+2)%3]);
                int handCnt[18];
                simState.copyCurrentCnt(handCnt);
                if (simState.isLeading()) {
                    action = getBestActionByPriority(
                        handCnt,
                        {},                         // 主动出牌无上家
                        currentPlayer,
                        simState.landlordRole,
                        currentPlayer,              // 主动时 lastActionPlayer 设为自己（无意义但参数需要）
                        myCards,
                        enemyCards
                    );
                }
                else {
                    action = getBestActionByPriority(
                        handCnt,
                        simState.getLastMove(),
                        currentPlayer,
                        simState.landlordRole,
                        simState.lastActionPlayer,  // 关键：上家是谁
                        myCards,
                        enemyCards
                    );
                }
                if (action.empty())action = {};
                simState.applyActionInPlace(action);
            }
        }
        if (stepCount >= MAX_SIM_STEPS) {
            // 平局处理：返回 0.5 或根据当前手牌数判断
            return false; // 保守视为失败
        }
        bool landlordWin = (simState.winner == simState.landlordRole);
        bool iAmLandlord = (simState.myRole == simState.landlordRole);
        return landlordWin == iAmLandlord;
    }

    // 把这次模拟结果往上反馈给父节点。
    // 如果这次模拟是赢，路径上的节点就都记一次“赢”；如果输了，就记一次“没赢”。
    void backpropagate(double result) {
        visits++;
        wins += result;
        if (parent) parent->backpropagate(result);
    }

    // 看当前节点的所有合法动作是不是都已经试过了。
    // 如果还有没试过的动作，就说明这个节点还可以继续扩展。
    bool isFullyExpanded() const {
         if (state->isGameOver) return true;
        // 直接使用本节点已存储的 actions，不再调用 getAllActions()
        if (actions.empty()) return true;
        for (const auto& act : actions) {
            bool hasChild = false;
            for (MCTSNode* child : children) {
                if (child->action == act) { hasChild = true; break; }
            }
            if (!hasChild) return false;
        }
        return true;
    }

    // 找到目前看起来最容易赢的孩子。
    // 这里看的是胜率，不是访问次数，所以更像“最终答案”而不是“继续探索的答案”。
    MCTSNode* bestChild() const {
        MCTSNode* best = nullptr;
        double bestWinRate = -1.0;
        for (MCTSNode* child : children) {
            if (child->visits == 0) continue;
            double winRate = (double)child->wins / child->visits;
            if (winRate > bestWinRate) {
                bestWinRate = winRate;
                best = child;
            }
        }
        if (!best && !children.empty()) best = children[0];
        return best;
    }

    // 取出当前节点对应的最佳动作。
    // 如果没有孩子，就返回空动作，表示过牌或者没有可用分支。
    vector<int> getBestAction() const {
        MCTSNode* best = bestChild();
        if (best) return best->action;
        return {};   // 无合法动作时返回空（过牌）
    }

    // 做一次完整的 MCTS：往下选、扩展、模拟、回传结果。
    // 这是 MCTS 的一整轮工作，重复很多次之后，树就会越来越接近“哪个动作更好”的答案。
    void iterate() {
        MCTSNode* node = this;
        while (!node->state->isGameOver && node->isFullyExpanded()) {
            node = node->selectChild();
            if (!node) break;   // 安全保护
        }

        if (!node) return;

        if (!node->state->isGameOver) {
            node->expand();

            if (!node->children.empty())
                node = node->children.back();
        }
        bool win = node->simulate();
        node->backpropagate(win ? 1.0 : 0.0);
    }
    // 牌很少时，直接用精确搜索，不再靠随机模拟。
    // 这是一个“该算清楚的时候就算清楚”的开关，避免在小残局里还靠蒙。
    bool isUseExactSearch(const GameState& s) {
        int total = s.totalCards[0] + s.totalCards[1] + s.totalCards[2];
        return total <= 8 || s.totalCards[s.currentPlayer] <= 3;  // 放宽条件
    }
    // 判断 actionA 和 actionB 是否使用了相同的点数（有交集）
    // 输入：两个按点数排序的 vector<int>（例如 [3,3,3,5]）
    // 输出：true 表示有重叠（即至少有一张牌的点数相同）
        bool hasOverlap(const vector<int>& a, const vector<int>& b) {
            // 利用双指针合并两个有序数组，检查是否有相同元素
            int i = 0, j = 0;
            while (i < (int)a.size() && j < (int)b.size()) {
                if (a[i] == b[j]) return true;
                else if (a[i] < b[j]) ++i;
                else ++j;
            }
            return false;
        }
        vector<vector<int>> deriveChildActions(const vector<vector<int>>& parentActions,
            const vector<int>& move) {
            vector<vector<int>> childActions;
            childActions.reserve(parentActions.size());
            for (const auto& act : parentActions) {
                if (!hasOverlap(act, move)) {
                    childActions.push_back(act);
                }
            }
            // 注意：pass（空数组）会在 hasOverlap 中与任何非空动作都不重叠，所以会被保留。
            // 如果需要特殊处理 pass（例如不允许连续 pass），可以在生成后根据游戏上下文额外过滤。
            return childActions;
        }
};

static int chooseWorldCount(const GameState& rootState) {
    int totalLeft = rootState.totalCards[0] + rootState.totalCards[1] + rootState.totalCards[2];
    if (totalLeft <= 8) return 14;
    if (totalLeft <= 12) return 12;
    if (totalLeft <= 20) return 10;
    if (totalLeft <= 30) return 8;
    return 6;
}


vector<int> decideWithParticleFilter(GameState& rootState, int timeMs = 900) {
    // 最终决策入口：先猜对手手牌，再对多个可能局面跑 MCTS，最后合并结果。
    int minEnemy = min(rootState.totalCards[(rootState.currentPlayer + 1) % 3], rootState.totalCards[(rootState.currentPlayer + 2) % 3]);
    int currentPlayer = rootState.currentPlayer;
    int myCards = rootState.totalCards[currentPlayer];
    if (ParticleFilter::particles.empty()) {
        int handCnt[18];
        rootState.copyCurrentCnt(handCnt);
        if (rootState.isLeading()) {
            return getBestActionByPriority( 
                handCnt,
                {},                         // 主动出牌无上家
                rootState.myRole,
                rootState.landlordRole,
                currentPlayer,              // 主动时 lastActionPlayer 设为自己（无意义但参数需要）
                myCards,
                minEnemy
            );
        }
        return getBestActionByPriority(
            handCnt,
            rootState.getLastMove(),
            rootState.myRole,
            rootState.landlordRole,
            rootState.lastActionPlayer,  // 关键：上家是谁
            myCards,
            minEnemy
        );
    }

    int N_Worlds = chooseWorldCount(rootState);
    int itersPerWorld = max(2600, timeMs * 60);
    vector<int> bestAction;
    double bestWinRate = -1.0;
    map<vector<int>, int> actionVisits;
    map<vector<int>, int> actionWins;
    int totalSims = 0;
    auto globalDeadline = chrono::steady_clock::now() + chrono::milliseconds(timeMs);
    for (int w = 0; w < N_Worlds; w++) {
        if (chrono::steady_clock::now() > globalDeadline)break;
        OpponentParticle p = ParticleFilter::sample();
        GameState worldState = rootState;
        int opp1 = (worldState.myRole + 1) % 3;
        int opp2 = (worldState.myRole + 2) % 3;
        worldState.setPlayerCnt(opp1, p.hand1);
        worldState.setPlayerCnt(opp2, p.hand2);
        MCTSNode rootNode(worldState);
        auto worldDeadline = chrono::steady_clock::now() + chrono::milliseconds(timeMs / N_Worlds);
        for (int iter = 0; iter < itersPerWorld && chrono::steady_clock::now() < worldDeadline; ++iter) {
            rootNode.iterate();
            ++totalSims;
        }
        
        for (MCTSNode* child : rootNode.children) {
            actionVisits[child->action] += child->visits;
            actionWins[child->action] += child->wins;
        }
    }
       cerr << "Total simulations: " << totalSims << endl;
    for (auto& [act, visits] : actionVisits) {
        if (visits == 0) continue;
        double rate = actionWins[act] / (double)visits;
        if (rate > bestWinRate) {
            bestWinRate = rate;
            bestAction = act;
        }
    }
    return bestAction;   // 点数序列
}
// ------ 静态成员定义（放在全局） ------
int ParticleFilter::myRole = 0;
int ParticleFilter::opp1Id = -1;
int ParticleFilter::opp2Id = -1;
int ParticleFilter::finalBid = 0;
std::vector<int> ParticleFilter::bidHistory;
std::vector<int> ParticleFilter::ownInitialHand;
std::vector<int> ParticleFilter::livePool;
int ParticleFilter::remainOpp1 = 0;
int ParticleFilter::remainOpp2 = 0;
std::vector<OpponentParticle> ParticleFilter::particles;
// ---------- 主函数入口 ----------//
// 这里负责三件事：读入 Botzone 的 JSON、还原当前牌局、输出我们这一步要出的牌。
int main() {
    auto start = chrono::steady_clock::now();
    string line, all;
    while (getline(cin, line)) all += line;
    Json::Reader reader;
    Json::Value input;
    if (!reader.parse(all, input)) {
        cerr << "Failed to parse JSON" << endl;
        return 1;
    }

    int turnID = input["responses"].size();
    Json::Value request = input["requests"][turnID];

    // ---------- 叫牌阶段 ----------
    // 如果这轮是叫分阶段，就不做出牌搜索，只根据手牌好坏决定叫几分。
    if (request.isMember("bid")) {
         vector<int> hand;
        if (request.isMember("own")) {
            for (Json::UInt i = 0; i < request["own"].size(); ++i)
                hand.push_back(request["own"][i].asInt());
        }

        vector<int> bidHistory;
        if (request["bid"].isArray()) {
            for (Json::UInt i = 0; i < request["bid"].size(); ++i)
                bidHistory.push_back(request["bid"][i].asInt());
        }

        // 计算当前最高叫分
        // 这样就知道我们是不是有机会超过别人。
        int currentMaxBid = 0;
        for (int b : bidHistory) if (b > currentMaxBid) currentMaxBid = b;

        // 是否是最后一个叫分玩家（即2号玩家）
        // 最后一个叫分的人可以根据前面两家的结果，稍微调整策略。
        bool isLast = (bidHistory.size() == 2);
        bool isFirst = (bidHistory.size() == 0);
        int bid = CardPatternAnalysis::decideBid(hand, currentMaxBid, isLast, isFirst);

        Json::Value ret;
        ret["response"] = bid;
        ret["data"] = input["data"];
        Json::FastWriter writer;
        cout << writer.write(ret) << endl;
        return 0;
    }

// ---------- 出牌阶段 ----------
// 叫牌结束后，就进入真正的出牌阶段：先把历史复原出来，再让搜索器决定这一手怎么打。
vector<int> fullHand;            // 自己初始完整手牌（含底牌）
vector<int> publicCard;
int myPosition = -1, landlordPosition = -1;
int finalBid = 0;

int firstPlayReq = -1;// 从请求序列里找到第一条真正开始出牌的请求，方便还原整个牌局。
for (int i = 0; i <= turnID; ++i) {
    Json::Value req = input["requests"][i];
    if (req.isMember("publiccard") && req.isMember("landlord")) {
        if (firstPlayReq == -1) firstPlayReq = i;
        landlordPosition = req["landlord"].asInt();
        myPosition = req["pos"].asInt();
        publicCard.clear();
        for (Json::UInt j = 0; j < req["publiccard"].size(); ++j)
            publicCard.push_back(req["publiccard"][j].asInt());
        fullHand.clear();
        for (Json::UInt j = 0; j < req["own"].size(); ++j)
            fullHand.push_back(req["own"][j].asInt());
        if (landlordPosition == myPosition) {
            for (int c : publicCard) fullHand.push_back(c);
        }
        if (req.isMember("finalbid")) finalBid = req["finalbid"].asInt();
        break;
    }
}
std::vector<int> bidHistory;
for (int i = 0; i <= turnID; ++i) {
    Json::Value req = input["requests"][i];
    if (req.isMember("bid")) {
        for (Json::UInt j = 0; j < req["bid"].size(); ++j)
            bidHistory.push_back(req["bid"][j].asInt());
    }
}
for(int b:bidHistory){
    cerr << "Bid:" << b << endl;
}
// 2. 收集动作序列
// 这里把历史动作按时间顺序整理出来，后面会按这个顺序重放到 GameState 里。
vector<vector<int>> allMoves;
vector<int> myPlayed;           // 自己打出的牌ID
vector<int> movePlayers;        // 对应 allMoves 中每个动作的玩家
int simPlayer = landlordPosition; // 当前轮到谁（地主开始）
bool firstRealMove = false;

// 地主视角手动补第一手出牌（仅当已经出过牌时）
// 这是为了处理 Botzone 历史里“第一手动作”和“当前请求”之间的时间顺序差异。
if (myPosition == landlordPosition && turnID >= 2) {
    Json::Value firstResp = input["responses"][1];
    if (firstResp.isArray() && firstResp.size() > 0) {
        vector<int> move;
        for (Json::UInt j = 0; j < firstResp.size(); ++j) {
            int card = firstResp[j].asInt();
            move.push_back(CardPatternAnalysis::getCardValue(card));
            myPlayed.push_back(card);
        }
        allMoves.push_back(move);
    } else {
        allMoves.push_back({});        // 保险
    }
    movePlayers.push_back(landlordPosition);
    simPlayer = (landlordPosition + 1) % 3;
    firstRealMove = true;
}

for (int i = firstPlayReq; i <= turnID; ++i) {
    Json::Value req = input["requests"][i];
    Json::Value hist = req["history"];

    // 跳过地主首回合全空占位请求
    // 这条记录只是占位，不是真正的历史动作。
    bool isFirstReq = (req.isMember("publiccard") && hist[0u].empty() && hist[1u].empty());
    if (isFirstReq) continue;

    // 处理上上家、上家
    // Botzone 的 history 里通常会给出最近两家的动作，这里按时间顺序还原成标准历史。
    for (int k = 0; k < 2; ++k) {
        // 跳过首个占位空（农民首次请求的 history[0] 为空占位）
        if (!firstRealMove && k == 0 && hist[0u].empty() && !hist[1u].empty()) {
            continue;
        }
        vector<int> move;
        for (Json::UInt j = 0; j < hist[k].size(); ++j) {
            int card = hist[k][j].asInt();
            move.push_back(CardPatternAnalysis::getCardValue(card));
        }
        allMoves.push_back(move);
        movePlayers.push_back(simPlayer);
        simPlayer = (simPlayer + 1) % 3;
        if (!move.empty()) firstRealMove = true;
    }

    // 已完成回合中目标玩家的响应（i < turnID）
    // 这里把已经结算完的响应也补进历史里，保证局面可以完整回放。
    if (i < turnID) {
        // 避免地主首出被重复收集（已手动补过）
        if (myPosition == landlordPosition && i == firstPlayReq && turnID >= 2) {
            // 跳过，因为已经通过手动补牌加入
        } else {
            Json::Value resp = input["responses"][i];
            if (resp.isArray() && resp.size() > 0) {
                vector<int> move;
                for (Json::UInt j = 0; j < resp.size(); ++j) {
                    int card = resp[j].asInt();
                    move.push_back(CardPatternAnalysis::getCardValue(card));
                    if (simPlayer == myPosition) myPlayed.push_back(card);
                }
                allMoves.push_back(move);
            } else {
                allMoves.push_back({});   // 过牌
            }
            movePlayers.push_back(simPlayer);
            simPlayer = (simPlayer + 1) % 3;
            if (!allMoves.back().empty()) firstRealMove = true;
        }
    }
}
// 3. 生成自己当前手牌（用于构造我的初始手牌，传给 GameState）
// 这是“我现在还剩什么牌”的真实版本：先拿到完整手牌，再把我已经打出去的牌删掉。
vector<int> myCurrentHand = fullHand;
for (int c : myPlayed) {
    auto it = find(myCurrentHand.begin(), myCurrentHand.end(), c);
    if (it != myCurrentHand.end()) myCurrentHand.erase(it);
}
// int k=1;
// for(auto& move:allMoves){
//     cerr << "Move"<<k++<<":";
//     for(int c:move){
//         cerr << c << " ";
//     }
//     cerr << endl;
// }

// 4. 构造 GameState 并重放历史
// 对手手牌未知，暂时给空。
// 先建一个“空白牌局快照”，再把历史动作一条条放进去，最后就得到当前真实局面。
vector<int> initHands[3];
initHands[myPosition] = fullHand;
initHands[(myPosition + 1) % 3] = {};
initHands[(myPosition + 2) % 3] = {};
vector<vector<int>> emptyHistory;
GameState rootState(initHands, publicCard, emptyHistory, landlordPosition, myPosition);

// 设置对手总牌数（初始17/20，后续 apply 会扣除）
// 这一步是为了让状态知道：每个人当前大概还剩多少张牌。
for (int p = 0; p < 3; ++p) {
    if (p == myPosition) continue;
    rootState.totalCards[p] = (p == landlordPosition) ? 20 : 17;
}
 
for(const auto& move : allMoves) {
    rootState.applyActionInPlace(move);
}

// 5. 粒子滤波初始化（它会根据 rootState 的已知信息生成粒子）
// 然后再根据对手已经出的牌筛掉不合理的猜测，最后在剩下的猜测里跑 MCTS。
// === 设置 ParticleFilter 需要的上下文 ===
ParticleFilter::myRole = myPosition;
ParticleFilter::finalBid = finalBid;   // 需从第一个出牌请求中获取（见下方获取方式）
ParticleFilter::bidHistory = bidHistory; // 需提前收集叫分序列（思路：遍历 requests 找含 "bid" 的请求）
ParticleFilter::ownInitialHand.clear();
for (int c : myCurrentHand)
    ParticleFilter::ownInitialHand.push_back(CardPatternAnalysis::getCardValue(c));

// 初始化粒子（此时不扣除历史，只扣除己方手牌和底牌）
    ParticleFilter::initialize(rootState);

// 按时间顺序，用所有对手/队友的历史动作更新粒子
for (size_t idx = 0; idx < allMoves.size(); ++idx) {
    int player = movePlayers[idx];
    if (player != myPosition && !allMoves[idx].empty()) {
        ParticleFilter::update(allMoves[idx], player);
    }
}
// 如果现在还没轮到我，就直接返回空动作。
if (rootState.currentPlayer != myPosition) {
    Json::Value ret;
    ret["response"] = Json::arrayValue;
    ret["data"] = input["data"];
    Json::FastWriter writer;
    cout << writer.write(ret) << endl;
    return 0;
}

// 真正决定这一手怎么打。
vector<int> bestAction = decideWithParticleFilter(rootState, 880);
// 搜索器给的是“点数序列”，Botzone 要的是“具体牌号”，所以这里要把点数映射回真实牌。
vector<int> cardMove;
vector<int> tempHand = myCurrentHand;
for (int val : bestAction) {
    auto it = find_if(tempHand.begin(), tempHand.end(), [val](int c) {
        return CardPatternAnalysis::getCardValue(c) == val;
    });
    if (it != tempHand.end()) {
        cardMove.push_back(*it);
        tempHand.erase(it);
    } else {
        cardMove.clear();
        break;
    }
}

// 结束时把结果打包成 JSON 输出。
// response 里放的是具体牌号列表，data 原样带回去，方便 Botzone 保留会话信息。
Json::Value ret;
Json::Value output(Json::arrayValue);
for (int c : cardMove) output.append(c);
ret["response"] = output;
ret["data"] = input["data"];
Json::FastWriter writer;
cout << writer.write(ret) << endl;
auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count();
cerr << "Elapsed time: " << elapsed << " ms" << endl;
return 0;
}
 