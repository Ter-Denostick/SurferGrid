#include "grdfileop.h"
#include <fstream>

GrdData GrdFileOp::GRDread(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        throw std::string("read file error: cannot open path " + path);
    }

    GrdData data;
    // 1 char - 1 байт; для чтения текста "DSSA" из grd 6 или "42525344"(dex) из grd 7
    char magic[4];
    ifs.read(magic, 4);
    if (!ifs) throw std::string("read file error: file too small");

    // Surfer 6
    if (magic[0] == 'D' && magic[1] == 'S' && magic[2] == 'A' && magic[3] == 'A') {
        ifs.close();
        ifs.open(path);
        std::string id;
        ifs >> id;

        ifs >> data.nCol >> data.nRow;
        ifs >> data.xLL >> data.xSize;
        ifs >> data.yLL >> data.ySize;
        ifs >> data.zMin >> data.zMax;

        double xHi = data.xSize;
        double yHi = data.ySize;
        data.xSize = (data.nCol > 1) ? (xHi - data.xLL) / (data.nCol - 1) : 0;
        data.ySize = (data.nRow > 1) ? (yHi - data.yLL) / (data.nRow - 1) : 0;
        data.blankValue = 1.70141e38;

        size_t totalPoints = static_cast<size_t>(data.nRow) * data.nCol;
        data.zValues.resize(totalPoints);
        for (size_t i = 0; i < totalPoints; ++i) {
            ifs >> data.zValues[i];
        }
    }
    // Surfer 7
    else {
        ifs.seekg(0, std::ios::beg);
        int32_t version = 1;

        while (ifs.peek() != EOF) {
            int32_t tagId = 0;
            int32_t tagSize = 0;
            ifs.read(reinterpret_cast<char*>(&tagId), sizeof(tagId));
            ifs.read(reinterpret_cast<char*>(&tagSize), sizeof(tagSize));
            if (!ifs) break;

            std::streampos nextTagPos = ifs.tellg() + static_cast<std::streamoff>(tagSize);

            switch (tagId) {
            case 0x42525344:
                ifs.read(reinterpret_cast<char*>(&version), sizeof(version));
                break;
            case 0x44495247:
                ifs.read(reinterpret_cast<char*>(&data.nRow), sizeof(data.nRow));
                ifs.read(reinterpret_cast<char*>(&data.nCol), sizeof(data.nCol));
                ifs.read(reinterpret_cast<char*>(&data.xLL), sizeof(data.xLL));
                ifs.read(reinterpret_cast<char*>(&data.yLL), sizeof(data.yLL));
                ifs.read(reinterpret_cast<char*>(&data.xSize), sizeof(data.xSize));
                ifs.read(reinterpret_cast<char*>(&data.ySize), sizeof(data.ySize));
                ifs.read(reinterpret_cast<char*>(&data.zMin), sizeof(data.zMin));
                ifs.read(reinterpret_cast<char*>(&data.zMax), sizeof(data.zMax));

                ifs.seekg(nextTagPos - static_cast<std::streamoff>(sizeof(double)));
                ifs.read(reinterpret_cast<char*>(&data.blankValue), sizeof(data.blankValue));
                break;
            case 0x41544144: {
                size_t totalPoints = static_cast<size_t>(data.nRow) * data.nCol;
                data.zValues.resize(totalPoints);
                ifs.read(reinterpret_cast<char*>(data.zValues.data()), totalPoints * sizeof(double));
                break;
            }
            case 0x49544c46: {
                int32_t nTraces = 0;
                int32_t nVertices = 0;
                ifs.read(reinterpret_cast<char*>(&nTraces), sizeof(nTraces));
                ifs.read(reinterpret_cast<char*>(&nVertices), sizeof(nVertices));

                data.faultTraces.resize(nTraces);
                ifs.read(reinterpret_cast<char*>(data.faultTraces.data()), nTraces * sizeof(GrdTrace));

                data.faultVertices.resize(nVertices);
                ifs.read(reinterpret_cast<char*>(data.faultVertices.data()), nVertices * sizeof(GrdVertex));
                break;
            }
            default:
                break;
            }
            ifs.seekg(nextTagPos, std::ios::beg);
        }
    }
    ifs.close();
    return data;
}

void GrdFileOp::GRDwrite(const std::string& path, const GrdData& data) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) {
        throw std::string("write file error: cannot create/open path " + path);
    }

    // тэг
    int32_t headerTagId = 0x42525344;
    int32_t headerTagSize = sizeof(int32_t);
    int32_t version = 2; // Используем версию 2 - строгое сравнение с BlankValue

    ofs.write(reinterpret_cast<const char*>(&headerTagId), sizeof(headerTagId));
    ofs.write(reinterpret_cast<const char*>(&headerTagSize), sizeof(headerTagSize));
    ofs.write(reinterpret_cast<const char*>(&version), sizeof(version));

    //сетка
    int32_t gridTagId = 0x44495247;
    int32_t gridTagSize = 72; // Размер секции параметров 72 байта

    ofs.write(reinterpret_cast<const char*>(&gridTagId), sizeof(gridTagId));
    ofs.write(reinterpret_cast<const char*>(&gridTagSize), sizeof(gridTagSize));

    // Записываем параметры геометрии сетки
    ofs.write(reinterpret_cast<const char*>(&data.nRow), sizeof(data.nRow));
    ofs.write(reinterpret_cast<const char*>(&data.nCol), sizeof(data.nCol));
    ofs.write(reinterpret_cast<const char*>(&data.xLL), sizeof(data.xLL));
    ofs.write(reinterpret_cast<const char*>(&data.yLL), sizeof(data.yLL));
    ofs.write(reinterpret_cast<const char*>(&data.xSize), sizeof(data.xSize));
    ofs.write(reinterpret_cast<const char*>(&data.ySize), sizeof(data.ySize));
    ofs.write(reinterpret_cast<const char*>(&data.zMin), sizeof(data.zMin));
    ofs.write(reinterpret_cast<const char*>(&data.zMax), sizeof(data.zMax));


    // вращение у нас не используется, поэтому записываем туда 0.0
    double rotation = 0.0;
    ofs.write(reinterpret_cast<const char*>(&rotation), sizeof(rotation));
    ofs.write(reinterpret_cast<const char*>(&data.blankValue), sizeof(data.blankValue));


    // данные
    int32_t dataTagId = 0x41544144;
    size_t totalPoints = static_cast<size_t>(data.nRow) * data.nCol;
    int32_t dataTagSize = static_cast<int32_t>(totalPoints * sizeof(double));

    // проверка реального размерв вектора с nRow * nCol
    if (data.zValues.size() < totalPoints) {
        throw std::string("write file error: zValues vector size is less than nRow * nCol");
    }

    ofs.write(reinterpret_cast<const char*>(&dataTagId), sizeof(dataTagId));
    ofs.write(reinterpret_cast<const char*>(&dataTagSize), sizeof(dataTagSize));

    ofs.write(reinterpret_cast<const char*>(data.zValues.data()), dataTagSize);


    // разломы
    if (!data.faultTraces.empty() && !data.faultVertices.empty()) {
        int32_t faultTagId = 0x49544c46;

        int32_t nTraces = static_cast<int32_t>(data.faultTraces.size());
        int32_t nVertices = static_cast<int32_t>(data.faultVertices.size());

        int32_t faultTagSize = sizeof(int32_t) + sizeof(int32_t) +
                               static_cast<int32_t>(nTraces * sizeof(GrdTrace)) +
                               static_cast<int32_t>(nVertices * sizeof(GrdVertex));

        ofs.write(reinterpret_cast<const char*>(&faultTagId), sizeof(faultTagId));
        ofs.write(reinterpret_cast<const char*>(&faultTagSize), sizeof(faultTagSize));

        ofs.write(reinterpret_cast<const char*>(&nTraces), sizeof(nTraces));
        ofs.write(reinterpret_cast<const char*>(&nVertices), sizeof(nVertices));

        ofs.write(reinterpret_cast<const char*>(data.faultTraces.data()), nTraces * sizeof(GrdTrace));
        ofs.write(reinterpret_cast<const char*>(data.faultVertices.data()), nVertices * sizeof(GrdVertex));
    }

    ofs.close();
}
