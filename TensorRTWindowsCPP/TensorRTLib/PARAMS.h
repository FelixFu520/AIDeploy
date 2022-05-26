#pragma once
#include <iostream>
#include <string>
#include <vector>


// \! ������������
enum NetWorkType
{
	LUSTER_CLS = 0,  //!< ��������
	LUSTER_SEG = 1,  //!< �ָ�����
	LUSTET_DET = 2,  //!< �������
	LUSTER_SIM = 3,  //!< ˫�����ƶ�����
	LUSTER_ANOMALY = 4,  //!< �쳣�������
};

// \! �����������
enum ErrorCode {
	LY_OK = 0,               // ����ִ�гɹ�
	LY_WRONG_CALL = 1,       // ����ĵ��ã�����˵�ָ�ģ�͵����˷���Ľӿ�
	LY_WRONG_FILE = 2,       // �ļ��Ҳ���������
	LY_WRONG_IMG = 3,        // ����ͼ���ʽ������߲�����
	LY_WRONG_CUDA = 4,       // cudaָ����������Կ�������
	LY_WRONG_TNAME = 5,      // caffe������inputTensorNames����outputTensorNames���ô���
	LY_WRONG_PLAT = 6,       // ƽ̨���ͺ�ģ���ļ���ƥ��
	LY_WRONG_BATCH = 7,      // onnx��batch sizeС��ʵ��ʹ�õ�
	LY_WRONG_GPUMEM = 8,       // �Դ治�㣬�޷������Կ��ڴ�
	LY_UNKNOW_ERROR = 99,    // δ֪��������ϵ�����߽��
};

// \! �����ʼ�� ����
struct Params
{
	NetWorkType netType = LUSTER_CLS;	            // ��������
	std::string onnxFilePath;	                    // onnx�ļ�·��
	std::string engineFilePath;	                    // �����ļ�����λ�ã����������Զ����ɣ��������Զ�����
	std::string log_path = "./";					// log��־����·��������Ϊ���򲻱�����־�ļ�
	bool fp16{ false };	                            // �Ƿ�ʹ�ð뾫�� ʹ�ð뾫�����������ļ��Ƚ�����ʹ�õ�ʱ���
	std::vector<float> stdValue{ 1.f, 1.f, 1.f };	// ��һ��ʱ�õ��ķ��superAIѵ����ģ�Ͳ���Ҫ����stdValue��meanValue��ʹ�ô�Ĭ��ֵ����
	std::vector<float> meanValue{ 0.f, 0.0, 0.0 };	// ��һ��ʱ�õ��ľ�ֵ������ѵ����ʽ���ܻ���Ҫ�˲�����
	int gpuId{ 0 };		                            // GPU ID��0��ʾ��һ���Կ���1��ʾ��2���Կ�....�Դ����ơ�
	int maxThread = 8;				                // mContexts���������������Ϊһ��ģ��֧��ͬʱ��������໭����������16�Ļ������ø��������������Ҫ����

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
// CoreImage

// \! �쳣��ⷵ�ؽ��
// CoreImage