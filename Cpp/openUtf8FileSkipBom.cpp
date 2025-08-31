#include <iostream>
#include <fstream>
#include <string>

/// UTF-8 BOMをスキップしてifstreamを返す関数
std::ifstream openUtf8FileSkipBom(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);  // バイナリで開く
    if (!ifs) {
        throw std::runtime_error("ファイルを開けません: " + path);
    }

    // 先頭3バイトをチェック
    unsigned char bom[3] = {0};
    ifs.read(reinterpret_cast<char*>(bom), 3);

    if (ifs.gcount() == 3) {
        if (!(bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)) {
            // BOMでなければ読み戻す
            ifs.seekg(0);
        }
    } else {
        // 3バイト未満なら先頭に戻す
        ifs.clear();
        ifs.seekg(0);
    }

    return ifs;
}

int main() {
    try {
        std::ifstream ifs = openUtf8FileSkipBom("test.txt");
        std::string line;
        while (std::getline(ifs, line)) {
            std::cout << line << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "エラー: " << e.what() << "\n";
    }
}
