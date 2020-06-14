Texture2D<float4> tex : register(t0); // 0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��

SamplerState smp : register(s0); // 0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[

// ���_�V�F�[�_�[����s�N�Z���V�F�[�_�[�ւ̂����Ɏg�p����\����
struct Output
{
	float4 svpos  : SV_POSITION; // �V�X�e���p���_���W
	float4 normal : NORMAL;      // �@���x�N�g��
	float2 uv     : TEXCOORD;    // uv�l�i�e�N�X�`����̍��W 0.0f�`1.0f�j
};

// �萔�o�b�t�@
cbuffer cbuff0 : register(b0)
{
	matrix world; // ���[���h�ϊ��s��
	matrix viewproj; // �r���[�v���W�F�N�V�����ϊ��s��
};

cbuffer Material : register(b1)
{
	float4 diffuse;  // �f�B�t���[�Y�F
	float4 specular; // �X�y�L����
	float3 ambient;	 // �A���r�G���g
};