#include <iostream>
#include <string> 
#include <stdexcept>
#include <iomanip>
#include <random>
#include <vector>
#include <algorithm>
#include <future>

class Board {
    public:
    static const int size = 19;
    private:
    char storage[size*size] = {};

    inline static bool checkCoord(int x) {
        return 0 <= x && x < size;
    }

    inline static bool checkPos(int pos) {
        return 0 <= pos && pos < size * size;
    }

    inline static bool checkCoords(int x, int y) {
        return checkCoord(x) && checkCoord(y);
    }

    inline static int flatten(int x, int y) {
        return y * size + x;
    }

    inline bool checkForWin(int x, int y) const {
        char s = storage[flatten(x, y)];
        if (s == '.') {
            return false;
        }

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= dx && dy <= 0; ++dy) {
                if (dx == 0 && dy == 0) {
                    continue;
                }
                int inRow = 0;
                for (int k = -4; k < 5; ++k) {
                    int nx = x + dx * k;
                    int ny = y + dy * k;
                    if (!checkCoords(nx, ny)) {
                        // can be either on start or at the end
                        continue;
                    }
                    int pos = flatten(nx, ny);
                    if (storage[pos] == s) {
                        ++inRow;
                        if (inRow == 5) {
                            return true;
                        }
                    } else {
                        inRow = 0;
                    }
                }
            }
        }
        return false;
    }
    
public:
    Board() {
        clear();
    }

    bool makeMove(char s, int x, int y) {
        int pos = flatten(x, y);
        if (!checkPos(pos) || storage[pos] != '.') {
            throw std::runtime_error("cannot make this move (" 
                + std::to_string(x)
                + ", "
                + std::to_string(y)
                + ")");
        }
        storage[pos] = s;
        return checkForWin(x, y);
    }

    char at(int x, int y) const {
        int pos = flatten(x, y);
        if (!checkPos(pos)) {
            return '-';
        }
        return storage[pos];
    }

    void clear() {
        for (int i = 0; i < size * size; ++i) {
            storage[i] = '.';
        }
    }
};

std::ostream& operator<<(std::ostream &ostream, const Board& board) {
    ostream << '\n' << "     ";
    for (int i = 0; i < board.size; ++i) {
        ostream << std::setfill('0') << std::setw(2) << i << ' ';
    }
    ostream << '\n' << "   --";
    for (int i = 0; i < board.size; ++i) {
        ostream << "---";
    }
    ostream << '\n';
    for (int y = 0; y < board.size; ++y) {
        ostream << std::setfill('0') << std::setw(2) << y << " | ";
        for (int x = 0; x < board.size; ++x) {
            ostream << board.at(x, y) << "  ";
        }
        ostream << "\n";
    }
    return ostream;
}

class NN5Player {
    float w[3][25];
    float a[3];
    float wr[3];
    float ar;
    char s;
    char opposite;
    static const int size = 5;

    static float randf() {
        float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        return (r - 0.5) / 10;
    }

    static float chanceOrZero(float x, float prob) {
        float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        if (r <= prob) {
            return x;
        } 
        return 0;
    }

public:
    NN5Player() {
        ar = randf() * 20;
        for (int i = 0; i < 3; i++) {
            a[i] = randf() * 20;
            wr[i] = randf();
            for (int j = 0; j < size * size; j++) {
                w[i][j] = randf();
            }
        }
    }

    NN5Player(std::istream& istream) {
        istream >> ar;
        for (int i = 0; i < 3; i++) {
            istream >> a[i]; 
            istream >> wr[i];
            for (int j = 0; j < size * size; j++) {
                istream >> w[i][j];
            }
        }
    }

    void init(char s) {
        this->s = s;
        opposite = (s == 'x') ? 'o' : 'x';
    }

    bool makeMove(Board& b) const {
        float maxr = -INFINITY;
        int xr = 9;
        int yr = 9;
        char map[3] = { '.', s, opposite};
        
        for (int x = 0; x < b.size; ++x) {
            for (int y = 0; y < b.size; ++y) {
                if (b.at(x, y) != '.') {
                    continue;
                }
                float rr = ar;
                for (int i = 0; i < 3; ++i) {
                    float r = a[i];
                    for (int dy = 0; dy < size; ++dy) {
                        for (int dx = 0; dx < size; ++dx) {
                            int match = b.at(x + dx - size / 2, y + dy - size / 2) == map[i];
                            r += w[i][dy * size + dx] * match;
                        }
                    }
                    if (r < 0) {
                        r = 0; // rectifier
                    }
                    rr += wr[i] * r;
                }
                if (rr > maxr) {
                    maxr = rr;
                    xr = x;
                    yr = y;
                }
            }
        }

        return b.makeMove(s, xr, yr);
    }

// deprecated
    NN5Player makeChild() const {
        NN5Player child = *this;

        child.ar += randf() / 100;
        for (int i = 0; i < 3; i++) {
            child.a[i] += randf() / 100;
            child.wr[i] += randf() / 200;
            for (int j = 0; j < 25; j++) {
                child.w[i][j] += randf() / 200;
            }
        }

        return std::move(child);
    }

    void mutateFrom (const NN5Player& other, float ac, float wc, float prob = 1) {
        ar = other.ar + chanceOrZero(randf() * ac, prob);
        for (int i = 0; i < 3; i++) {
            a[i] = other.a[i] + chanceOrZero(randf() * ac, prob);
            wr[i] = other.wr[i] + chanceOrZero(randf() * wc, prob);
            for (int j = 0; j < size * size; j++) {
                w[i][j] = other.w[i][j] + chanceOrZero(randf() * wc, prob);
            }
        }
    }

    friend std::ostream& operator<<(std::ostream &ostream, NN5Player& pet);
};

struct NN5 {
    NN5Player pet;
    int score = 0;
};

std::ostream& operator<<(std::ostream &ostream, NN5Player& pet) {
    ostream << pet.ar << ' ';
    for (int i = 0; i < 3; i++) {
        ostream << pet.a[i] << ' '; 
        ostream << pet.wr[i] << ' ';
        for (int j = 0; j < 25; j++) {
            ostream << pet.w[i][j] << ' ';
        }
    }
    return ostream;
}

class RandomPlayer {
    char s;
    public:
    RandomPlayer(char s) :s(s) {}

    bool makeMove(Board& b) {
        for (int i = 0; i < 10; ++i) {
            int x = rand() % b.size;
            int y = rand() % b.size;
            if (b.at(x, y) == '.') {
                return b.makeMove(s, x, y);
            }
        }
        for (int y = 0; y < b.size; ++y) {
            for (int x = 0; x < b.size; ++x) {
                if (b.at(x, y) == '.') {
                    return b.makeMove(s, x, y);
                }
            }
        }
        throw std::runtime_error("end");
    }
};

using namespace std;

void playGame(Board& board, NN5& p1, NN5& p2) {
    p1.pet.init('x');
    p2.pet.init('o');

    bool res;
    for (int i = 0; i < board.size * board.size; ++i) {
        res = p1.pet.makeMove(board);
        ++i;
        if (res == true) {
            p1.score += 3;
            p2.score -= 5;
            return;
        }
        if (i == board.size * board.size) {
            return;
        }
        res = p2.pet.makeMove(board);
        if (res == true) {
            p1.score -= 5;
            p2.score += 3;
            return;
        }
    }
}

int main() {
    int playersSize = 50;
    vector<Board> boards;
    for (int i = 0; i < playersSize; ++i) {
        boards.emplace_back();
    }
    vector<string> saved;
    vector<future<void>> games;
    saved.push_back("0.327774 0.369878 0.450327 1.02738 0.534242 -0.164047 0.372477 0.991542 -0.0707307 -0.429781 -0.602136 0.799149 -0.596347 0.416658 0.157025 1.12457 0.672127 0.803299 0.947895 -0.185917 -1.55886 0.398459 0.198491 0.503183 -0.45957 0.406067 -0.362999 0.303053 1.57264 -1.46705 0.2127 -0.195918 -0.0576506 -0.34862 -0.0577229 -0.186233 0.173106 -1.34051 0.501074 0.592258 0.661387 0.219854 -0.667208 -0.0982067 0.450269 0.246146 0.542797 -0.523668 -0.125958 1.5274 -0.282043 -0.204072 -0.123275 0.0973385 0.954026 0.295124 0.185144 0.520004 -0.614836 0.0241557 -0.438257 -0.321052 -1.11799 0.0394086 0.411401 -0.327549 0.233376 -0.0725183 0.638288 -0.225793 -0.67306 -1.25012 -0.366095 -0.370602 -0.075261 0.513555 -1.33457 -0.0751719 -0.712558 -0.401011 -0.819926 -0.761058");
    saved.push_back("0.377333 0.694129 0.0879142 -0.192383 0.270975 0.391913 -0.102868 0.167182 -0.110878 -0.297537 -0.0650761 0.0480116 -0.0328395 0.0580826 -0.0432072 -0.0368954 -0.162114 0.137697 0.214949 -0.0928879 -0.17387 0.0140973 0.266592 -0.0413043 0.0949899 0.366492 0.202079 0.149296 0.462311 -0.401133 -0.092523 -0.150863 -0.289902 -0.0996087 0.0817541 0.00607323 -0.0626963 -0.556114 -0.0117449 0.301426 0.0413498 0.278179 -0.245563 0.0435103 0.191262 -0.20377 0.0916148 -0.245852 -0.161073 0.401564 0.0162601 -0.0699425 -0.129339 -0.112148 0.193753 0.0625802 0.0793439 0.0908321 -0.345052 -0.15621 -0.105301 -0.202575 -0.00207274 -0.550203 0.392816 0.134681 -0.12913 -0.0348803 -0.216432 -0.0158752 -0.158455 -0.0200823 0.147304 0.0509829 0.498428 0.243566 -0.187787 0.0250426 -0.277286 0.186141 -0.298393 -0.226026");
    saved.push_back("0.374087 0.68702 0.106283 -0.22161 0.241526 0.392572 -0.102396 0.172801 -0.0786245 -0.335137 -0.0825826 0.0618816 -0.0418635 0.0270462 -0.0686142 -0.0372926 -0.161556 0.136171 0.189846 -0.0825906 -0.136611 0.00411206 0.265645 -0.0405114 0.0361222 0.326953 0.17565 0.156536 0.469365 -0.432276 -0.0529391 -0.16944 -0.282373 -0.131655 0.0939502 -0.0158474 -0.0340835 -0.573446 -0.0280463 0.362292 0.0464173 0.244371 -0.230826 0.00955379 0.216417 -0.15444 0.0572612 -0.220138 -0.173269 0.372107 0.0522712 -0.104881 -0.138781 -0.0770839 0.189608 0.0598102 0.0748687 0.120258 -0.316122 -0.139524 -0.137019 -0.224061 0.0260364 -0.574392 0.399284 0.0849866 -0.17524 -0.0297998 -0.163459 0.0117141 -0.182508 0.00795794 0.171085 0.101666 0.492512 0.232503 -0.147774 0.0747727 -0.292636 0.183957 -0.27812 -0.175365 ");
    saved.push_back("0.381229 0.702524 0.0925914 -0.184368 0.28283 0.391292 -0.108265 0.160786 -0.0893504 -0.30473 -0.064777 0.052021 -0.043697 0.0654222 -0.0543311 -0.0371143 -0.149051 0.131985 0.215901 -0.0805013 -0.179644 0.0338364 0.249606 -0.0359966 0.107793 0.376642 0.194271 0.143139 0.462543 -0.412349 -0.0823024 -0.138478 -0.275933 -0.114711 0.0721541 0.00189671 -0.078419 -0.571111 -0.0223174 0.307231 0.0191594 0.297906 -0.235748 0.0404624 0.188328 -0.205079 0.0883481 -0.235723 -0.138094 0.399249 0.00877511 -0.0527375 -0.141915 -0.106619 0.216266 0.0626557 0.0950228 0.0755366 -0.350983 -0.169834 -0.131812 -0.198285 -0.00331174 -0.562745 0.382004 0.123842 -0.115849 -0.0314148 -0.212269 -0.0131665 -0.149184 -0.00810913 0.139739 0.0400678 0.507353 0.263067 -0.200011 0.0307011 -0.277657 0.166895 -0.29505 -0.233882 ");
    saved.push_back("0.906681 -0.122742 0.641795 1.75765 2.32782 -0.775406 -0.102903 1.36141 -0.344129 0.19509 0.179552 2.21271 0.460076 -2.52599 -1.58828 1.67176 1.48414 0.329867 -0.416279 -0.418521 -0.706344 -2.40887 -0.257078 0.42277 -1.77157 0.180762 -1.30424 2.53703 0.940585 1.84769 3.25522 -0.0825207 0.26032 -0.75136 -0.00808376 -0.158806 0.462815 -1.77049 1.36767 1.23937 -1.4773 -1.0274 -0.223605 -0.490911 -0.603195 1.15164 0.111692 1.02713 -0.47941 1.38433 -0.141976 0.155706 1.49449 0.575739 1.21323 0.759729 -0.219275 0.376153 -1.62371 -0.739793 -1.51401 0.773158 -1.06446 -0.592587 0.770119 -2.46653 -0.0927016 1.22315 -1.63902 1.01036 0.888328 -1.13827 0.581785 -0.876805 -0.614913 -0.089214 -2.10131 1.39848 1.01246 2.17782 -0.727789 -2.23765 ");
    saved.push_back("0.384426 0.693918 0.0965765 -0.21439 0.246416 0.395986 -0.120838 0.175517 -0.088683 -0.302252 -0.0479603 0.042209 -0.0290174 0.0845613 -0.0349279 -0.0539698 -0.173807 0.138642 0.199507 -0.0898182 -0.1685 0.0206337 0.284261 -0.0430712 0.0725183 0.341842 0.204314 0.163921 0.460354 -0.426974 -0.109481 -0.150456 -0.301376 -0.122905 0.105154 -0.0149602 -0.0969189 -0.565684 -0.00944303 0.334398 0.042064 0.312597 -0.233393 0.0330466 0.207158 -0.19832 0.0790768 -0.256648 -0.139678 0.388584 0.0235332 -0.0581774 -0.120783 -0.102206 0.188751 0.0629416 0.0796363 0.103066 -0.314343 -0.146789 -0.103908 -0.203824 -0.00724466 -0.545284 0.413271 0.137909 -0.160244 -0.00364799 -0.207774 0.000312516 -0.13925 -0.0257102 0.152909 0.0673574 0.497336 0.220067 -0.180963 0.026413 -0.296581 0.169359 -0.295003 -0.193048 ");
    saved.push_back("0.613326 -0.0312059 0.0310647 -0.0867011 -0.00734206 0.861136 -0.500164 -0.42696 0.842896 -1.3392 0.0480767 -0.0182788 -0.104577 0.351656 -0.305961 -0.131559 0.415635 0.360307 -0.614109 -0.27237 -1.34385 0.159725 -0.450565 -0.543365 -0.0940525 -0.306129 0.743754 0.106177 -0.801976 -0.128377 0.149857 -0.199021 1.19198 0.31112 0.561607 -0.316463 -0.319555 -0.228432 0.212666 -0.273559 -0.276599 -0.650302 -0.111607 -0.727655 0.303978 0.984871 0.222401 0.100827 0.214498 -0.554551 0.0950973 -0.303366 -0.42332 -0.197489 -0.150729 -1.32851 0.00961343 -0.990395 -0.653029 -0.12752 -0.347131 0.115198 -0.476076 0.583133 0.534304 -0.804398 -0.442861 -0.250456 -0.0496852 -0.156602 -0.346363 0.0990314 -0.0466706 -0.614831 0.380178 -1.00979 -0.508375 0.434941 -0.184584 0.0827146 0.784706 0.637552 ");
    saved.push_back("0.41395 0.689679 0.198131 -0.231166 0.0794834 0.46259 -0.0872399 0.152409 -0.0435107 -0.27672 -0.310514 0.219605 0.0100456 0.0704494 0.00120143 0.167955 -0.0604335 -0.00302427 0.414772 -0.212199 -0.0112394 -0.0438274 0.399314 -0.0254601 0.259902 0.385803 0.161183 0.18827 0.450803 -0.338086 -0.150273 -0.155024 -0.345485 -0.142141 0.33381 0.123998 -0.369552 -0.68343 -0.273593 0.396938 0.0898504 0.505893 -0.0734571 0.209896 -0.043658 -0.392255 -0.102149 -0.0140914 -0.115279 0.502624 0.16371 -0.186518 -0.0912038 0.0330236 0.228022 0.0731248 0.108826 -0.0907952 -0.113031 -0.0564353 -0.158882 -0.253303 0.028541 -0.524367 0.494556 0.322352 -0.320369 -0.0349967 -0.331078 -0.113962 -0.218059 -0.0112277 -0.0280684 0.0584838 0.368668 0.338274 0.0362283 0.19652 -0.0158178 -0.00905014 -0.330857 -0.352108 ");
    saved.push_back("0.906681 -0.122742 0.641795 1.75765 2.32782 -0.775406 -0.102903 1.36141 -0.344129 0.19509 0.179552 2.21271 0.460076 -2.52599 -1.58828 1.67176 1.48414 0.329867 -0.416279 -0.418521 -0.706344 -2.40887 -0.257078 0.42277 -1.77157 0.180762 -1.30424 2.53703 0.940585 1.84769 3.25522 -0.0825207 0.26032 -0.75136 -0.00808376 -0.158806 0.462815 -1.77049 1.36767 0.842877 -1.4773 -1.0274 -0.223605 -0.490911 -0.603195 1.15164 0.111692 1.02713 -0.47941 1.38433 -0.141976 0.155706 1.49449 0.575739 1.21323 0.759729 -0.219275 0.376153 -1.62371 -0.739793 -1.51401 0.773158 -1.06446 -0.566945 0.770119 -2.46653 -0.0927016 1.22315 -1.63902 1.01036 0.888328 -1.13827 0.581785 -0.876805 -0.614913 -0.089214 -2.10131 1.39848 1.01246 2.17782 -0.727789 -2.23765 ");
    saved.push_back("0.824021 0.19918 -0.601512 1.53379 0.537986 -0.252596 2.06577 0.653583 0.985223 -0.865421 -0.240195 0.687608 -1.01205 -0.165774 -0.500835 0.432162 0.685202 0.616821 0.456287 0.675281 -1.01081 0.223752 0.0383103 0.0702935 -0.518038 1.22295 0.0481228 1.06189 1.8207 -0.12296 0.16079 -0.136489 -0.719086 -0.145596 -0.114924 -0.273871 0.323736 -1.01831 1.49359 0.755757 0.803841 0.301981 -0.181566 -0.897901 1.0829 1.24111 -0.0222967 -0.946365 -0.611359 2.49156 -0.296525 -0.241906 0.765871 0.159816 0.802981 1.04205 -0.354099 0.165507 -1.7989 0.506497 -0.767507 0.531588 -0.909326 0.0855844 0.785129 -0.277982 0.222134 -0.204044 1.0681 -0.185723 -0.89696 -1.38054 -0.680156 -0.953172 0.460451 -0.160778 -2.41585 1.4577 -1.15915 -0.530325 -1.8491 -0.932241 ");
    saved.push_back("0.357189 0.19918 -0.741963 1.53379 0.537986 -0.544535 2.06577 0.653583 0.593937 -0.421554 -0.240195 0.687608 -1.01205 -0.165774 -0.500835 0.432162 0.685202 0.616821 0.521157 0.675281 -1.01081 0.328911 0.0383103 0.0702935 -0.518038 1.22295 -0.422245 1.4135 1.8207 0.367977 0.16079 -0.136489 -0.274693 -0.145596 -0.114924 -0.273871 0.323736 -1.01831 1.49359 0.755757 0.803841 0.301981 -0.667208 -0.897901 1.0829 1.0279 -0.0222967 -0.619518 -0.62918 2.49156 -0.296525 -0.241906 0.327197 0.340763 0.802981 1.04205 -0.354099 0.165507 -1.7989 0.506497 -0.510564 0.531588 -0.789147 0.143487 0.785129 -0.277982 0.222134 -0.204044 1.0796 -0.185723 -0.89696 -1.38054 -0.680156 -0.953172 0.64844 -0.160778 -2.41585 1.26316 -1.15915 -0.530325 -1.8491 -0.613803 ");

    vector<NN5> pets;

    for (int i = 0; i < saved.size(); ++i) {
        auto t = std::istringstream(saved[i]);
        pets.push_back({NN5Player(t), 0});
    }

    for (int i = saved.size(); i < playersSize; ++i) {
        pets.emplace_back();
    }

    for (int epoch = 0; epoch < 700; ++epoch) {
        for (int i = 0; i < pets.size(); ++i) {
            pets[i].score = 0;
        }

        for (int i = 0; i < pets.size(); ++i) {
            games.clear();
            for (int j = 0; j < pets.size(); ++j) {
                games.push_back(async(launch::async, [&](int i, int j){
                    if (i == j) {
                        return;
                    }
                    playGame(boards[j], pets[i], pets[j]);
                }, i, j));
            }
            for (int j = 0; j < pets.size(); ++j) {
                games[j].get();
                boards[j].clear();
            }
        }

        sort(pets.begin(), pets.end(), [](const NN5& a, const NN5& b){ return a.score > b.score; });

        games.clear();
        for (int i = 0; i < pets.size() / 2; ++i) {
            games.push_back(async(launch::async, [&](int i) {
                pets[i + pets.size() / 2].pet.mutateFrom(pets[i].pet, 10, 10, 0.02);
            }, i));
        }
        for (int i = 0; i < pets.size() / 2; ++i) {
            games[i].get();
        }
    }

    playGame(boards[0], pets[0], pets[1]);
    cout << boards[0];
    boards[0].clear();

    for (int i = 0; i < 5; ++i) {
        cout << "\n\n\npet #" << i << "\n";
        cout << pets[i].pet;
    }

    return 0;
}