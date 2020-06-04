#pragma once

#include <Windows.h>
#include <string>
#include <assert.h>
#include <d3d12.h>
#include <vector>

namespace
{
	// @brief �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
// @param format �t�H�[�}�b�g�i%d �Ƃ� %f �Ƃ��́j
// @param �ϒ�����
// @remarks ���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�܂���
	void DebugOutputFormatString(const char* format, ...)
	{
#ifdef _DEBUG
		va_list valist;
		va_start(valist, format);
		printf(format, valist);
		va_end(valist);
#endif
	}

	// �ʓ|�����Ǐ����Ȃ���΂Ȃ�Ȃ��֐�
	LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		// �E�B���h�E���j�����ꂽ��Ă΂��
		if (msg == WM_DESTROY)
		{
			PostQuitMessage(0); // OS�ɑ΂��āu�������̃A�v���͏I���v�Ɠ`����
			return 0;
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	size_t AlignmentedSize(size_t size, size_t alignment)
	{
		return size + alignment - size % alignment;
	}

	// ���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂�
	// @param modelPath �A�v���P�[�V�������猩��pmd���f���̃p�X
	// @param texPath PMD���f�����猩���e�N�X�`���̃p�X
	// @return �A�v���P�[�V�������猩���e�N�X�`���̃p�X
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

	// std::string�i�}���`�o�C�g������j����std::wstring�i���C�h������j�𓾂�
	// @param str �}���`�o�C�g������
	// @return �ϊ����ꂽ���C�h������
	std::wstring GetWideStringFromString(const std::string& str)
	{
		// �Ăяo��1��ځi�����񐔂𓾂�j
		auto num1 = MultiByteToWideChar(
			CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(),
			-1,
			nullptr,
			0
		);

		std::wstring wstr; // string��wcahr_t��
		wstr.resize(num1);

		// �Ăяo��2��ځi�m�ۍς݂�wstr�ɕϊ���������R�s�[�j
		auto num2 = MultiByteToWideChar(
			CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(),
			-1,
			&wstr[0],
			num1
		);

		assert(num1 == num2); // �ꉞ�`�F�b�N

		return wstr;
	}

	// �t�@�C��������g���q���擾����
	// @param path �Ώۂ̃p�X������
	// @return �g���q
	std::string GetExtension(const std::string& path)
	{
		int idx = path.rfind('.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	// �e�N�X�`���̃p�X���Z�p���[�^�[������ŕ�������
	// @param path �Ώۂ̃p�X������
	// @param splitter ��؂蕶��
	// @return �����O��̕�����y�A
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

		debugLayer->EnableDebugLayer(); // �f�o�b�O���C���[��L��������
		debugLayer->Release(); // �L����������C���^�[�t�F�[�X���������
	}

	std::vector<float> GetGaussianWeights(size_t count, float s)
	{
		std::vector<float> weights(count); // �E�F�C�g�z��ԋp�p
		float x = 0.0f;
		float total = 0.0f;

		for (auto& wgt : weights)
		{
			wgt = expf(-(x * x) / (2 * s * s));
			total += wgt;
			x += 1.0f;
		}

		total = total * 2.0f - 1;

		// ������ 1 �ɂȂ�悤�ɂ���
		for (auto& wgt : weights)
		{
			wgt /= total;
		}
		
		return weights;
	}
}