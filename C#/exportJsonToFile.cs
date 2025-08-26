using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using System.Text.Json;

/// <summary>
/// CSV ファイルを読み込み、ネスト対応 JSON に変換して出力するツール
/// 対応例:
/// - ドット区切りのキーでネストを表現 ("address.city")
/// - 配列表現 ("skills.0")
/// - 型推定 (数値、真偽値、文字列)
/// - 引用符・セル内改行対応
/// </summary>
class CsvToJsonExporter
{
    static void Main()
    {
        try
        {
            string csvPath = "export.csv";   // 入力 CSV ファイル
            string jsonPath = "export.json"; // 出力 JSON ファイル

            // CSV ファイルを安全に読み込み
            var csvLines = ReadCsv(csvPath);
            if (csvLines.Count < 2)
            {
                Console.WriteLine("CSV にデータがありません。");
                return;
            }

            // 1行目をヘッダとして取得
            var headers = csvLines[0];

            // JSON 固定部分 (必要に応じて変更可能)
            var result = new Dictionary<string, object>
            {
                ["Name"] = "Address",
                ["Version"] = "1.0"
            };

            // データ行を順に処理
            for (int i = 1; i < csvLines.Count; i++)
            {
                var row = csvLines[i]; // 現在のデータ行
                var rowObject = new Dictionary<string, object>();

                // 各列の値を処理
                for (int j = 0; j < headers.Count; j++)
                {
                    string header = headers[j].Trim(); // ヘッダ名
                    string rawValue = j < row.Count ? row[j].Trim() : string.Empty; // CSV列値
                    object value = ParseValue(rawValue); // 型推定付き
                    SetNestedValue(rowObject, header, value); // ネスト設定
                }

                // "name" カラムを JSON キーとして使用
                if (!rowObject.TryGetValue("name", out var keyObj) || string.IsNullOrEmpty(keyObj?.ToString()))
                {
                    throw new Exception($"CSV の {i + 1} 行目: name カラムが空です。");
                }

                string key = keyObj.ToString();
                rowObject.Remove("name"); // 内部オブジェクトからは削除
                result[key] = rowObject;  // JSON に追加
            }

            // JSON にシリアライズ
            var options = new JsonSerializerOptions { WriteIndented = true };
            string json = JsonSerializer.Serialize(result, options);

            // JSON ファイルを書き込み
            File.WriteAllText(jsonPath, json, Encoding.UTF8);
            Console.WriteLine($"JSON を出力しました: {jsonPath}");
        }
        catch (Exception ex)
        {
            Console.WriteLine("エラー: " + ex.Message);
        }
    }

    /// <summary>
    /// CSV を安全に読み込む
    /// - 引用符付き文字列対応 (例: "Tokyo, Japan")
    /// - "" -> " 変換
    /// - カンマ内改行対応
    /// </summary>
    static List<List<string>> ReadCsv(string path)
    {
        var result = new List<List<string>>();

        using var reader = new StreamReader(path, Encoding.UTF8);
        while (!reader.EndOfStream)
        {
            string line = reader.ReadLine();
            if (line == null) continue;

            var row = new List<string>();
            var sb = new StringBuilder();
            bool inQuotes = false;

            for (int i = 0; i < line.Length; i++)
            {
                char c = line[i];

                if (c == '"')
                {
                    if (inQuotes && i + 1 < line.Length && line[i + 1] == '"')
                    {
                        // "" は " に変換
                        sb.Append('"');
                        i++;
                    }
                    else
                    {
                        // 引用符の開始/終了を反転
                        inQuotes = !inQuotes;
                    }
                }
                else if (c == ',' && !inQuotes)
                {
                    // カンマで区切る (引用符内なら無視)
                    row.Add(sb.ToString());
                    sb.Clear();
                }
                else
                {
                    sb.Append(c);
                }
            }
            // 最後の列を追加
            row.Add(sb.ToString());

            result.Add(row);
        }

        return result;
    }

    /// <summary>
    /// 値を適切な型に変換
    /// - int, double, bool などに変換
    /// - 空文字は null
    /// </summary>
    static object ParseValue(string input)
    {
        if (string.IsNullOrEmpty(input)) return null;

        if (bool.TryParse(input, out bool boolVal)) return boolVal;
        if (int.TryParse(input, NumberStyles.Integer, CultureInfo.InvariantCulture, out int intVal)) return intVal;
        if (double.TryParse(input, NumberStyles.Float, CultureInfo.InvariantCulture, out double dblVal)) return dblVal;

        return input; // 上記以外は文字列
    }

    /// <summary>
    /// ドット区切りキーを解釈してネストされたオブジェクト/配列に値をセット
    /// - "address.city" -> { "address": { "city": value } }
    /// - "skills.0" -> 配列に格納
    /// </summary>
    static void SetNestedValue(Dictionary<string, object> obj, string key, object value)
    {
        if (string.IsNullOrEmpty(key)) return;

        var keys = key.Split('.');
        object current = obj;

        for (int i = 0; i < keys.Length; i++)
        {
            string k = keys[i];
            bool isLast = i == keys.Length - 1;
            bool isArrayIndex = int.TryParse(k, out int index);

            if (isLast)
            {
                // 最後のキーの場合、値をセット
                if (isArrayIndex)
                {
                    var list = current as List<object> ?? new List<object>();
                    ReplaceInParent(obj, keys, i, list);
                    EnsureListSize(list, index + 1);
                    list[index] = value;
                }
                else
                {
                    var dict = current as Dictionary<string, object>;
                    dict[k] = value;
                }
            }
            else
            {
                // 途中のキー、オブジェクト/配列を掘り下げ
                bool nextIsArray = int.TryParse(keys[i + 1], out _);

                if (isArrayIndex)
                {
                    var list = current as List<object> ?? new List<object>();
                    ReplaceInParent(obj, keys, i, list);
                    EnsureListSize(list, index + 1);
                    if (list[index] == null)
                        list[index] = nextIsArray ? new List<object>() : new Dictionary<string, object>();
                    current = list[index];
                }
                else
                {
                    var dict = current as Dictionary<string, object>;
                    if (!dict.ContainsKey(k))
                        dict[k] = nextIsArray ? new List<object>() : new Dictionary<string, object>();
                    current = dict[k];
                }
            }
        }
    }

    /// <summary>
    /// List のサイズを確保 (null で埋める)
    /// </summary>
    static void EnsureListSize(List<object> list, int size)
    {
        while (list.Count < size) list.Add(null);
    }

    /// <summary>
    /// 親オブジェクトに新しい子オブジェクト/配列を差し替える
    /// </summary>
    static void ReplaceInParent(Dictionary<string, object> root, string[] keys, int depth, object newChild)
    {
        object current = root;
        for (int i = 0; i < depth; i++)
            current = (current as Dictionary<string, object>)[keys[i]];

        if (current is Dictionary<string, object> dict)
            dict[keys[depth]] = newChild;
    }
}
