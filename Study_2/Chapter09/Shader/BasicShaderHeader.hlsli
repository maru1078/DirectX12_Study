Texture2D<float4> tex : register(t0);  // 0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> sph : register(t1);  // 1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> spa : register(t2);  // 2�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> toon : register(t3); // 3�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��

SamplerState smp : register(s0);     // 0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[
SamplerState smpToon : register(s1); // 1�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[�i�g�D�[���p�j

// ���_�V�F�[�_�[����s�N�Z���V�F�[�_�[�ւ̂����Ɏg�p����\����
struct Output
{
	float4 svpos   : SV_POSITION; // �V�X�e���p���_���W
	float4 normal  : NORMAL0;     // �@���x�N�g��
	float4 vnormal : NORMAL1;     // �r���[�ϊ���̖@���x�N�g��
	float2 uv      : TEXCOORD;    // uv�l�i�e�N�X�`����̍��W 0.0f�`1.0f�j
	float3 ray     : VECTOR;      // �x�N�g��
};

// �萔�o�b�t�@
cbuffer cbuff0 : register(b0)
{
	matrix world; // ���[���h�ϊ��s��
	matrix view;  // �r���[�ϊ��s��
	matrix proj;  // �v���W�F�N�V�����ϊ��s��
	float3 eye;   // ���_
};

cbuffer Material : register(b1)
{
	float4 diffuse;  // �f�B�t���[�Y�F
	float4 specular; // �X�y�L����
	float3 ambient;	 // �A���r�G���g
};