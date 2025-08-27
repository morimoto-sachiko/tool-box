#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/// ===========================================================
/// CSV を読み込んで 2次元配列 (行×セル) として返す
/// - ダブルクォート対応（セル内のカンマや " を扱える）
/// ===========================================================
std::vector<std::vector<std::string>> ReadCsv(const std::string& path)
{
    std::vector<std::vector<std::string>> rows;
    std::ifstream ifs(path);
    if (!ifs.is_open())
        throw std::runtime_error("CSV ファイルを開けません: " + path);

    std::string line;
    while (std::getline(ifs, line))
    {
        std::vector<std::string> row;
        std::string cell;
        bool inQuotes = false;  // "～" 内にいるかどうか

        for (size_t i = 0; i < line.size(); i++)
        {
            char c = line[i];
            if (c == '"')
            {
                // "" → " に変換
                if (inQuotes && i + 1 < line.size() && line[i + 1] == '"')
                {
                    cell.push_back('"');
                    i++;
                }
                else
                {
                    inQuotes = !inQuotes;  // クォートの開始/終了
                }
            }
            else if (c == ',' && !inQuotes)
            {
                // 区切り文字（クォート外のみ）
                row.push_back(cell);
                cell.clear();
            }
            else
            {
                // 普通の文字
                cell.push_back(c);
            }
        }
        row.push_back(cell);  // 行末のセルを追加
        rows.push_back(row);
    }
    return rows;
}

/// ===========================================================
/// 文字列を最適な型に変換して json に格納
/// - 空文字 → null
/// - "true"/"false" → bool
/// - 数値文字列 → int / double
/// - その他 → string
/// ===========================================================
json ParseValue(const std::string& str)
{
    if (str.empty()) return nullptr;

    // bool 判定
    if (str == "true" || str == "TRUE") return true;
    if (str == "false" || str == "FALSE") return false;

    // int 判定
    try {
        size_t pos;
        int val = std::stoi(str, &pos);
        if (pos == str.size()) return val;
    } catch (...) {}

    // double 判定
    try {
        size_t pos;
        double val = std::stod(str, &pos);
        if (pos == str.size()) return val;
    } catch (...) {}

    // それ以外は文字列
    return str;
}

/// ===========================================================
/// ドット区切りのキーを解釈して、ネストした JSON に代入する
/// - "address.city" → { "address": { "city": value } }
/// - "skills.0"     → { "skills": [ value, ... ] }
/// ===========================================================
void SetNestedValue(json& j, const std::string& key, const json& value)
{
    std::istringstream ss(key);
    std::string token;
    json* current = &j;

    // キーを分解 (例: "address.city.zip" → ["address","city","zip"])
    std::vector<std::string> keys;
    while (std::getline(ss, token, '.'))
    {
        keys.push_back(token);
    }

    // ネストをたどりながら代入
    for (size_t i = 0; i < keys.size(); i++)
    {
        const std::string& k = keys[i];
        bool isLast = (i == keys.size() - 1);

        // 配列インデックスかどうか判定
        bool isArrayIndex = false;
        size_t index = 0;
        try {
            index = std::stoi(k);
            isArrayIndex = true;
        } catch (...) {}

        if (isLast)
        {
            // 最下層に到達 → 値を代入
            if (isArrayIndex)
            {
                if (!current->is_array()) *current = json::array();
                if (current->size() <= index) current->resize(index + 1);
                (*current)[index] = value;
            }
            else
            {
                (*current)[k] = value;
            }
        }
        else
        {
            // 途中ノードの処理
            const std::string& nextKey = keys[i + 1];
            bool nextIsIndex = false;
            try { std::stoi(nextKey); nextIsIndex = true; } catch (...) {}

            if (isArrayIndex)
            {
                if (!current->is_array()) *current = json::array();
                if (current->size() <= index) current->resize(index + 1);

                // 未初期化なら次の構造を作る
                if ((*current)[index].is_null())
                    (*current)[index] = nextIsIndex ? json::array() : json::object();

                current = &(*current)[index];
            }
            else
            {
                if (!(*current).contains(k))
                    (*current)[k] = nextIsIndex ? json::array() : json::object();

                current = &(*current)[k];
            }
        }
    }
}

/// ===========================================================
/// メイン処理
/// - CSV を読み込み
/// - 固定ヘッダ(Name/Version)を追加
/// - 各行を JSON オブジェクトに変換
/// - "name" カラムをキーとして result に格納
/// ===========================================================
int main()
{
    try
    {
        // 入出力ファイルパス
        std::string csvPath = "export.csv";
        std::string jsonPath = "export.json";

        // CSV 読み込み
        auto rows = ReadCsv(csvPath);
        if (rows.size() < 2)
        {
            std::cerr << "CSV にデータがありません\n";
            return 1;
        }

        auto headers = rows[0];  // 1行目をヘッダとして利用

        // 出力 JSON の固定部分
        json result;
        result["Name"] = "Address";  // 固定
        result["Version"] = "1.0";   // 固定

        // データ行の処理
        for (size_t i = 1; i < rows.size(); i++)
        {
            auto& row = rows[i];
            json obj = json::object();

            // 各セルをヘッダに基づいて obj に格納
            for (size_t j = 0; j < headers.size(); j++)
            {
                std::string header = headers[j];
                std::string rawValue = (j < row.size()) ? row[j] : "";
                json val = ParseValue(rawValue);
                SetNestedValue(obj, header, val);
            }

            // "name" をキーとして result に追加
            if (!obj.contains("name"))
                throw std::runtime_error("行 " + std::to_string(i + 1) + ": name カラムが空です");

            std::string key = obj["name"];
            obj.erase("name");   // name はキーに昇格させたので削除
            result[key] = obj;
        }

        // JSON をファイル出力
        std::ofstream ofs(jsonPath);
        ofs << result.dump(2) << std::endl;

        std::cout << "JSON を出力しました: " << jsonPath << std::endl;
    }
    catch (std::exception& ex)
    {
        std::cerr << "エラー: " << ex.what() << std::endl;
        return 1;
    }
}
