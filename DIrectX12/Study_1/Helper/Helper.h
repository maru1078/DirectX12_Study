#pragma once

#include <Windows.h>
#include <string>
#include <assert.h>
#include <d3d12.h>
#include <vector>

namespace
{
	// @brief コンソール画面にフォーマット付き文字列を表示
// @param format フォーマット（%d とか %f とかの）
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

	// 面倒だけど書かなければならない関数
	LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		// ウィンドウが破棄されたら呼ばれる
		if (msg == WM_DESTROY)
		{
			PostQuitMessage(0); // OSに対して「もうこのアプリは終わる」と伝える
			return 0;
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	size_t AlignmentedSize(size_t size, size_t alignment)
	{
		return size + alignment - size % alignment;
	}

	// モデルのパスとテクスチャのパスから合成パスを得る
	// @param modelPath アプリケーションから見たpmdモデルのパス
	// @param texPath PMDモデルから見たテクスチャのパス
	// @return アプリケーションから見たテクスチャのパス
	std::string GetTexturePathFromModelAndTexPath(
		const std::string& modelPath,
		const char* texPath
	)
	{
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
			0
		);

		std::wstring wstr; // stringのwcahr_t版
		wstr.resize(num1);

		// 呼び出し2回目（確保済みのwstrに変換文字列をコピー）
		auto num2 = MultiByteToWideChar(
			CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(),
			-1,
			&wstr[0],
			num1
		);

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

	// テクスチャのパスをセパレーター文字列で分離する
	// @param path 対象のパス文字列
	// @param splitter 区切り文字
	// @return 分離前後の文字列ペア
	std::pair<std::string, std::string> SplitFileName(
		const std::string& path, const char splitter = '*')
	{
		int idx = path.find(splitter);
		std::pair<std::string, std::string> ret;
		ret.first = path.substr(0, idx);
		ret.second = path.substr(idx + 1, path.length() - idx - 1);

		return ret;
	}

	void EnableDebugLayer()
	{
		ID3D12Debug* debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));

		debugLayer->EnableDebugLayer(); // デバッグレイヤーを有効化する
		debugLayer->Release(); // 有効化したらインターフェースを解放する
	}

	std::vector<float> GetGaussianWeights(size_t count, float s)
	{
		std::vector<float> weights(count); // ウェイト配列返却用
		float x = 0.0f;
		float total = 0.0f;

		for (auto& wgt : weights)
		{
			wgt = expf(-(x * x) / (2 * s * s));
			total += wgt;
			x += 1.0f;
		}

		total = total * 2.0f - 1;

		// 足して 1 になるようにする
		for (auto& wgt : weights)
		{
			wgt /= total;
		}
		
		return weights;
	}
}