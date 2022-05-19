/*****************************************************************************
* @author : FelixFu
* @date : 2021/10/10 14:40
* @last change :
* @description : ONNXRuntime Session
*****************************************************************************/
#pragma once
#include <queue>	// ϵͳ��
#include <opencv2/opencv.hpp> // opencv��
#include <onnxruntime_cxx_api.h>  //ONNXRuntime��
#include "params.h"	// �Զ�������ݽṹ�� ����Ľӿ�


// \! ------------------------------------Session Start------------------------------
class ORTSession {
public:
	ORTSession() {};	// Ĭ�Ϲ��캯��
	ORTSession(const Params& params, int& nErrnoFlag);  // ���湹�죬 ����Params�������ʹ������nErrorFlag
public:
	Ort::Session m_Session{ nullptr };	// Ort::Session ָ��
	// ����env��session�Ķ���Ҳ���и��ӣ���Ҫ��������Ĺ��캯���ڲ���
	//����Ҫ��������Ա����������ȫ�ֿ��ã�����infer()�õ�sessionʱ�ͻᱨ��:
	Ort::Env m_Env{ ORT_LOGGING_LEVEL_WARNING, "OnnxSession" };	

	// \! �����������
	std::vector<std::vector<int64_t>> mInputDims;  // ģ������ά��
	std::vector<std::vector<int64_t>> mOutputDims;    // ģ�����ά��
	std::vector<std::string> mOutputTensorNames;	// ģ�����������
	std::vector<std::string> mInputTensorNames; // ģ�����������

};
// \! ------------------------------------Session End------------------------------


// \! ִ�������Ľṹ��
struct ORTCore_ctx
{
	Params params;							// \! ִ�������ĵĳ�ʼ����
	std::shared_ptr<ORTSession> session;	// \! һ��ģ�Ͷ�Ӧһ��Session��һ��Sessionִ�������Ŀ���ͬʱ������̵߳���
};
