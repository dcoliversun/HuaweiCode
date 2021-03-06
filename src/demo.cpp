#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define TEST
#define _TIME_TEST
using namespace std;

struct Data {
    vector<double> features;
    int label;
    Data(vector<double> f, int l) : features(f), label(l) {}
};

struct Param {
    vector<double> wtSet;
};

class LR {
public:
    void train();
    void predict();
    int loadModel();
    int storeModel();
    LR(const char* trainFile, const char* testFile, string predictOutFile);

private:
    vector<Data> trainDataSet;
    vector<Data> testDataSet;
    vector<int> predictVec;
    Param param;
    const char* trainFile;
    const char* testFile;
    string predictOutFile;
    // TODO: 可以改进，改为程序中存储
    string weightParamFile = "modelweight.txt";

private:
    bool init();
    bool loadTrainData();
    bool loadTestData();
    int storePredict(vector<int> &predict);
    void initParam();
    double wxbCalc(const Data &data);
    double sigmoidCalc(const double wxb);
    double lossCal();
    // 随机梯度，可能也会改进的点
    double gradientSlope(const vector<Data> &dataSet, int index, const vector<double> &sigmoidVec);

private:
    int featuresNum;
    // 权重初始值
    const double wtInitV = 1.0;
    // 步长
    const double stepSize = 0.03;
    // 最大迭代次数
    const int maxIterTimes = 400;
    // 预测真实值阈值
    const double predictTrueThresh = 0.5;
    // 训练显示步长
    const int train_show_step = 50;
};

LR::LR(const char* trainF, const char* testF, string predictOutF) {
    trainFile = trainF;
    testFile = testF;
    predictOutFile = predictOutF;
    featuresNum = 0;
    init();
}

//bool LR::loadTrainData() {
//    ifstream infile(trainFile.c_str());
//    string line;
//
//    if (!infile) {
//        cout << "打开训练文件失败" << endl;
//        exit(0);
//    }
//
//    while (infile) {
//        getline(infile, line);
//        if (line.size() > featuresNum) {
//            stringstream sin(line);
//            char ch;
//            double dataV;
//            int i;
//            vector<double> feature;
//            i = 0;
//
//            while (sin) {
//                char c = sin.peek();
//                if (int(c) != -1) {
//                    sin >> dataV;
//                    feature.push_back(dataV);
//                    sin >> ch;
//                    ++i;
//                } else {
//                    cout << "训练文件数据格式不正确，出错行为" << (trainDataSet.size() + 1) << "行" << endl;
//                    return false;
//                }
//            }
//            int ftf;
//            ftf = (int)feature.back();
//            feature.pop_back();
//            trainDataSet.push_back(Data(feature, ftf));
//        }
//    }
//    infile.close();
//    return true;
//}

bool LR::loadTrainData() {
    char *file = NULL;
    int fd = open(trainFile, O_RDONLY);
    long size = lseek(fd, 0, SEEK_END);
    file = (char *) mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    char tmp[10];
    vector<double> feature;
    int i = 0, j = 0;
    while (j < size) {
        if (file[j] == '\n') {
            strncpy(tmp,file + i, j - i);
            int label = atoi(tmp);
            trainDataSet.push_back(Data(feature, label));
            feature.clear();
            i = j + 1;
        } else if (file[j] == ',') {
            strncpy(tmp, file + i, j - i);
            double f = atof(tmp);
            feature.push_back(f);
            i = j + 1;
        }
        ++j;
    }
    return true;
}

void LR::initParam() {
    int i;
    for (i = 0; i < featuresNum; ++i) {
        param.wtSet.push_back(wtInitV);
    }
}

bool LR::init() {
    trainDataSet.clear();
    bool status = loadTrainData();
    if (status != true) {
        return false;
    }
    featuresNum = trainDataSet[0].features.size();
    param.wtSet.clear();
    initParam();
    return true;
}

double LR::wxbCalc(const Data &data) {
    double mulSum = 0.0L;
    int i;
    double wtv, feav;
    for (i = 0; i < param.wtSet.size(); ++i) {
        wtv = param.wtSet[i];
        feav = data.features[i];
        mulSum += wtv * feav;
    }
    return mulSum;
}

inline double LR::sigmoidCalc(const double wxb) {
    double expv = exp(-1 * wxb);
    double expvInv = 1 / (1 + expv);
    return expvInv;
}

double LR::lossCal() {
    double lossV = 0.0L;
    int i;

    for (i = 0; i < trainDataSet.size(); ++i) {
        lossV -= trainDataSet[i].label * log(sigmoidCalc(wxbCalc(trainDataSet[i])));
        lossV -= (1 - trainDataSet[i].label) * log(1 - sigmoidCalc(wxbCalc(trainDataSet[i])));
    }
    lossV /= trainDataSet.size();
    return lossV;
}

double LR::gradientSlope(const vector<Data> &dataSet, int index, const vector<double> &sigmoidVec) {
    double gsV = 0.0L;
    int i;
    double sigv, label;
    for (i = 0; i < dataSet.size(); i++) {
        sigv = sigmoidVec[i];
        label = dataSet[i].label;
        gsV += (label - sigv) * (dataSet[i].features[index]);
    }
    gsV = gsV / dataSet.size();
    return gsV;
}

void LR::train() {
    double sigmoidVal;
    double wxbVal;
    int i, j;

    for (i = 0; i < maxIterTimes; i++) {
        vector<double> sigmoidVec;

        for (j = 0; j < trainDataSet.size(); j++) {
            wxbVal = wxbCalc(trainDataSet[j]);
            sigmoidVal = sigmoidCalc(wxbVal);
            sigmoidVec.push_back(sigmoidVal);
        }

        for (j = 0; j < param.wtSet.size(); j++) {
            param.wtSet[j] += stepSize * gradientSlope(trainDataSet, j, sigmoidVec);
        }
#ifdef TEST
        if (i % train_show_step == 0) {
            cout << "iter" << i << ". updated weight value is : ";
            for (j = 0; j < param.wtSet.size(); ++j) {
                cout << param.wtSet[j] << "  ";
            }
            cout << endl;
        }
#endif
    }
}

void LR::predict() {
    double sigVal;
    int predictVal;

    loadTestData();
    for (int j = 0; j < testDataSet.size(); ++j) {
        sigVal = sigmoidCalc(wxbCalc(testDataSet[j]));
        predictVal = sigVal >= predictTrueThresh ? 1 : 0;
        predictVec.push_back(predictVal);
    }

    storePredict(predictVec);
}

int LR::loadModel() {
    string line;
    int i;
    vector<double> wtTmp;
    double dbt;

    ifstream fin(weightParamFile.c_str());
    if (!fin) {
#ifdef TEST
        cout << "打开模型参数文件失败" << endl;
#endif
        exit(0);
    }

    getline(fin, line);
    stringstream sin(line);
    for (i = 0; i < featuresNum; i++) {
        char c = sin.peek();
        if (c == -1) {
#ifdef TEST
            cout << "模型参数数量少于特征数量，退出" << endl;
#endif
            return -1;
        }
        sin >> dbt;
        wtTmp.push_back(dbt);
    }
    param.wtSet.swap(wtTmp);
    fin.close();
    return 0;
}

int LR::storeModel() {
    string line;
    int i;

    ofstream fout(weightParamFile.c_str());
    if (!fout.is_open()) {
        cout << "打开模型参数文件失败" << endl;
    }
    if (param.wtSet.size() < featuresNum) {
        cout << "wtSet size is " << param.wtSet.size() << endl;
    }
    for (i = 0; i < featuresNum; i++) {
        fout << param.wtSet[i] << " ";
    }
    fout.close();
    return 0;
}

//bool LR::loadTestData() {
//    ifstream infile(testFile.c_str());
//    string lineTitle;
//
//    if (!infile) {
//#ifdef TEST
//        cout << "打开测试文件失败" << endl;
//#endif
//        exit(0);
//    }
//
//    while (infile) {
//        vector<double> feature;
//        string line;
//        getline(infile, line);
//        if (line.size() > featuresNum) {
//            stringstream sin(line);
//            double dataV;
//            int i;
//            char ch;
//            i = 0;
//            while (i < featuresNum && sin) {
//                char c = sin.peek();
//                if (int(c) != -1) {
//                    sin >> dataV;
//                    feature.push_back(dataV);
//                    sin >> ch;
//                    i++;
//                } else {
//#ifdef TEST
//                    cout << "测试文件数据格式不正确" << endl;
//#endif
//                    return false;
//                }
//            }
//            testDataSet.push_back(Data(feature, 0));
//        }
//    }
//
//    infile.close();
//    return true;
//}

bool LR::loadTestData() {
    char *file = NULL;
    int fd = open(testFile, O_RDONLY);
    long size = lseek(fd,0, SEEK_END);
    file = (char *) mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    char tmp[10];
    vector<double> feature;
    int i = 0, j = 0;
    while (j < size) {
        if (file[j] == '\n') {
            strncpy(tmp, file + i, j - i);
            double f = atof(tmp);
            feature.push_back(f);
            testDataSet.push_back(Data(feature, 0));
            feature.clear();
            i = j + 1;
        } else if (file[j] == ',') {
            strncpy(tmp, file + i, j - i);
            double f = atof(tmp);
            feature.push_back(f);
            i = j + 1;
        }
        ++j;
    }
    return true;
}

bool loadAnswerData(string awFile, vector<int> &awVec) {
    ifstream infile(awFile.c_str());
    if (!infile) {
#ifdef TEST
        cout << "打开答案文件失败" << endl;
#endif
        exit(0);
    }

    while (infile) {
        string line;
        int aw;
        getline(infile, line);
        if (line.size() > 0) {
            stringstream sin(line);
            sin >> aw;
            awVec.push_back(aw);
        }
    }

    infile.close();
    return true;
}

bool loadAnswerData(const char* awFile, vector<int>& awVec) {
    char *file = NULL;
    int fd = open(awFile, O_RDONLY);
    long size = lseek(fd, 0, SEEK_END);
    file = (char *) mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    char tmp[10];
    int i = 0, j = 0;
    while (j < size) {
        if (file[j] == '\n') {
            strncpy(tmp, file + i, j - i);
            int f = atoi(tmp);
            awVec.push_back(f);
            i = j + 1;
        }
        ++j;
    }
    return true;
}

int LR::storePredict(vector<int> &predict) {
    string line;
    int i;

    ofstream fout(predictOutFile.c_str());
    if (!fout.is_open()) {
        cout << "打开预测结果文件失败" << endl;
    }
    for (i = 0; i < predict.size(); i++) {
        fout << predict[i] << endl;
    }
    fout.close();
    return 0;
}

int main(int argc, char *argv[]) {
#ifdef _TIME_TEST
    clock_t start = clock();
#endif

    vector<int> answerVec;
    vector<int> predictVec;
    int correctCount;
    double accurate;
#ifdef TEST
    const char* trainFile = "/Users/oliver_sun/CLionProjects/HW_warmup_match/data/train_data.txt";
    const char* testFile = "/Users/oliver_sun/CLionProjects/HW_warmup_match/data/test_data.txt";
    string predictFile = "/Users/oliver_sun/CLionProjects/HW_warmup_match/projects/student/result.txt";

    const char* answerFile = "/Users/oliver_sun/CLionProjects/HW_warmup_match/data/answer.txt";
#else
    string trainFile = "/data/train_data.txt";
    string testFile = "/data/test_data.txt";
    string predictFile = "/projects/student/result.txt";
#endif

    LR logist(trainFile, testFile, predictFile);

    cout << "ready to train model" << endl;
    logist.train();

    cout << "training ends, ready to store the model" << endl;
    logist.storeModel();

#ifdef TEST
    cout << "ready to load answer data" << endl;
    loadAnswerData(answerFile, answerVec);
#endif

    cout << "let's have a prediction test" << endl;
    logist.predict();

#ifdef TEST
    loadAnswerData(predictFile, predictVec);
    cout << "test data set size is " << predictVec.size() << endl;
    correctCount = 0;
    for (int j = 0; j < predictVec.size(); j++) {
        if (j < answerVec.size()) {
            if (answerVec[j] == predictVec[j]) {
                correctCount++;
            }
        } else {
            cout << "answer size less than the real predicted value" << endl;
        }
    }
    accurate = ((double) correctCount) / answerVec.size();
    cout << "the prediction accuracy is " << accurate << endl;
#endif

#ifdef _TIME_TEST
    clock_t ends = clock();
    cout <<"Running Time : "<<(double)(ends - start)/ CLOCKS_PER_SEC << endl;
#endif

    return 0;
}