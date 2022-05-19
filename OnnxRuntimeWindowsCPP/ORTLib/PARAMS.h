#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

// \! ������������
enum NetWorkType
{
	FF_CLS = 0,  //!< ��������
	FF_SEG = 1,  //!< �ָ�����
	FF_DET = 2,  //!< �������
	FF_SIM = 3,  //!< ˫�����ƶ�����
	FF_ANOMALY = 4,  // �쳣�������.
};

// \! ���巵����
enum ErrorCode {
	FF_OK=0,				// OK
	FF_ERROR_PNULL=1,		// ָ��Ϊ��
	FF_ERROR_NETWORK=2,    // �������ʹ���
	FF_ERROR_INPUT=3,      // �����ʽ����
	FF_UNKNOW_ERROR=99,		// δ֪������Ҫ������Ա����
};

// \! �����ʼ������
struct Params
{
	NetWorkType netType = FF_ANOMALY;	            // ��������
	std::string onnxFilePath;	                    // onnx�ļ�·��
	std::string log_path = "./";					// log��־����·��������Ϊ���򲻱�����־�ļ�
	std::vector<float> stdValue{ 1.f, 1.f, 1.f };	// ��һ��ʱ�õ��ķ��superAIѵ����ģ�Ͳ���Ҫ����stdValue��meanValue��ʹ�ô�Ĭ��ֵ����
	std::vector<float> meanValue{ 0.f, 0.0, 0.0 };	// ��һ��ʱ�õ��ľ�ֵ������ѵ����ʽ���ܻ���Ҫ�˲�����
	int sessionThread = 1;							// ִ��onnx ����ʹ�õ��߳���
};

// \! ���෵�ؽ��
typedef std::pair<int, float> ClassifyResult;

// \! ��ⷵ�ؽ��
typedef struct BBox
{
	float x1, y1, x2, y2;     // ���Ͻǡ����½�����ֵ
	float det_confidence;     // �����а���Ŀ������Ŷ�
	float class_confidence;   // ����Ŀ�����Ϊ��class_id������Ŷ�
	unsigned int class_id;    // ���
}DetectResult;

// \! �ָ�ؽ��
//typedef CoreImage SegementResult;

// \! �쳣��ⷵ�ؽ��
//typedef CoreImage AnomalyResult;