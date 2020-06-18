#ifndef HELPER_H_
#define HELPER_H_

#include <string>
#include <assert.h>

// ★namespaceに入れたら「既に定義されています」みたいなエラーがなくなった・・・？
namespace
{

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			assert(0);
		}
	}

	// @brief コンソール画面にフォーマット付き文字列を表示
	// @param format フォーマット（%dとか%fとかの）
	// @param 可変長引数
	// @remarks この関数はデバッグ用です。デバッグ時にしか動作しません
	void DebugOutputFormatString(const char* format, ...)
	{
#ifdef _DEBUG

		va_list valist;
		va_start(valist, format);
		printf(format, valist);
		va_end(valist);

#endif
	}

	// アラインメントにそろえたサイズを返す
	// @param size 元のサイズ
	// @param alignment アラインメントサイズ
	// @return アラインメントをそろえたサイズ
	size_t AlignmentedSize(size_t size, size_t alignment)
	{
		if (size % alignment == 0) return size;

		return size + alignment - size % alignment;
	}

	// モデルのパスとテクスチャのパスから合成パスを得る
	// @param modelPath アプリケーションから見たpmdモデルパス
	// @texPath PMDモデルから見たテクスチャのパス
	// @return アプリケーションから見たテクスチャのパス
	std::string GetTexturePathFromModelAndTexPath(
		const std::string& modelPath, const char* texPath)
	{
		// ファイルのフォルダー区切りは「\」と「/」の2種類
		int pathIndex1 = modelPath.rfind('/');
		int pathIndex2 = modelPath.rfind('\\');

		auto pathIndex = max(pathIndex1, pathIndex2);
		auto folderPath = modelPath.substr(0, pathIndex + 1);

		return folderPath + texPath;
	}

	// std::string（マルチバイト文字列）からstd::wstring（ワイド文字列）を得る
	// @param str マルチバイト文字列
	// @return 変換されたワイド文字列
	std::wstring GetWideStringFromString(const std::string& str)
	{
		// 呼び出し1回目（文字列数を得る）
		auto num1 = MultiByteToWideChar(
			CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(),
			-1,
			nullptr,
			0);

		std::wstring wstr; // stringのwchar_t版
		wstr.resize(num1);

		// 呼び出し2回目（確保済みのwstrに変換文字列をコピー）
		auto num2 = MultiByteToWideChar(
			CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(),
			-1,
			&wstr[0],
			num1);

		assert(num1 == num2); // 一応チェック
		return wstr;
	}

	// ファイル名から拡張子を取得する
	// @param path 対象のパス文字列
	// @return 拡張子
	std::string GetExtension(const std::string& path)
	{
		int idx = path.rfind('.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	// テクスチャのパスをセパレーター文字で分割する
	// @param path 対象のパス文字列
	// @param splitter 区切り文字
	// @return 分割前後の文字列ペア
	std::pair<std::string, std::string> SplitFileName(
		const std::string& path, const char splitter = '*')
	{
		int idx = path.find(splitter);
		std::pair<std::string, std::string> ret;
		ret.first = path.substr(0, idx);
		ret.second = path.substr(idx + 1, path.length() - idx - 1);
		return ret;
	}
}

#endif // !HELPER_H_

