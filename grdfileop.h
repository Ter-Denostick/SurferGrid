#include <string>
#include "grddata.h"

class GrdFileOp {
public:
    static GrdData GRDread(const std::string& path);
    static void GRDwrite(const std::string& path, const GrdData& data);
};


