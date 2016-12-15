#include "../headFiles/main.h"
#include "../headFiles/network.h"

DEFINE_string(outputFileCoefficient, "./output/coeff.out", "output file name to store non-zero coefficient");
DEFINE_string(logFile, "./output/output.log", "log file to record objective value per freq");
DEFINE_int64(logFrequency, 1000, "objective value logging frequency");
DEFINE_double(learningRate, 0.01, "lambda to control sparsity on the output");
DEFINE_int64(maxIteration, 100, "Number of maximum training iteration");
DEFINE_string(dataTrainFile, "", "design matrix denoted as X : M by N matrix  ");
DEFINE_string(dataLabelFile, "", "observation vector denoted as Y : M by 1 matrix  ");
DEFINE_int64(columns, 0, "the number of columns of x files .. denoted as N ");
DEFINE_int64(samples, 0, "the number of rows of x files, rows of Y file .. denoted as M ");
DEFINE_string(algorithm, "lasso", " algoritm : lasso or logistic");

void printFlags() {
    std::cout << "FLAGS_outputFileCoefficient: " << FLAGS_outputFileCoefficient << std::endl;
    std::cout << "FLAGS_logFile: " << FLAGS_logFile << std::endl;
    std::cout << "FLAGS_logFrequency: " << FLAGS_logFrequency << std::endl;
    std::cout << "FLAGS_learningRate: " << FLAGS_learningRate << std::endl;
    std::cout << "FLAGS_maxIteration: " << FLAGS_maxIteration << std::endl;
    std::cout << "FLAGS_dataTrainFile: " << FLAGS_dataTrainFile << std::endl;
    std::cout << "FLAGS_dataLabelFile: " << FLAGS_dataLabelFile << std::endl;
    std::cout << "FLAGS_columns: " << FLAGS_columns << std::endl;
    std::cout << "FLAGS_samples: " << FLAGS_samples << std::endl;
    std::cout << "FLAGS_algorithm: " << FLAGS_algorithm << std::endl;
}

int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, false);

    //TODO: 初始化glog日志环境
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    LOG(INFO) << "finding role" << std::endl;
    network* aries = new network;
    aries->init(argc,argv);

    return 0;
}