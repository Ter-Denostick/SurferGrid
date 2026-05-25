#include <vector>

//подструктуры для разломов
struct GrdTrace {
    int32_t iFirst;
    int32_t nPts;
};

struct GrdVertex {
    double x;
    double y;
};

struct GrdData {
    int32_t nRow = 0;
    int32_t nCol = 0;
    double xLL = 0.0;
    double yLL = 0.0;
    double xSize = 0.0;
    double ySize = 0.0;
    double zMin = 0.0;
    double zMax = 0.0;
    double blankValue = 1.70141e38;

    std::vector<double> zValues;

    std::vector<GrdTrace> faultTraces;
    std::vector<GrdVertex> faultVertices;
};
