#pragma once
#include <cuda_runtime.h>  // cuda��
#include <cuda_runtime_api.h>
#include "loguru.hpp" // https://github.com/emilk/loguru

// \! ָ��device id��GPU�Ƿ����
bool IsCompatible(int device)
{
	cudaError_t st = cudaSetDevice(device);
	if (st != cudaSuccess)
		return false;

	cudaDeviceProp deviceProp;
	cudaGetDeviceProperties(&deviceProp, device);
	if (deviceProp.major < 3)
		return false;

	return true;
}

// \! ǰ���� �����ࡢ�ָ�쳣��⡢Ŀ����ı�׼������һ������
// \! ��1��8U->32F;(2)����ֵ��������;(3)�����3ͨ���ģ�BGR->RGB;(4)ת��һά����
// \@param buffers: ���棬tensorRT�е�samples����
// \@param cv_images: ����ͼƬ��openCV��ʽ
// \@param params: �����ṹ��
// \@param inputName: onnx����������
int normalization(const samplesCommon::BufferManager & buffers,std::vector<cv::Mat>& cv_images, const Params& params, std::string inputName)
{
	// 1.����host�ڴ�
	float* hostDataBuffer = static_cast<float*>(buffers.getHostBuffer(inputName));
	int nums = cv_images.size();	// ���ͼƬ����-batchsize
	int depth = cv_images[0].channels();	// ���channels
	int height = cv_images[0].rows;	// ���height
	int width = cv_images[0].cols;	// ���width

	if (depth == 1) {// ͨ����Ϊ1
		for (int n = 0, index = 0; n < nums; n++)
		{
			cv_images[n].convertTo(cv_images[n], CV_32F, 1.0 / 255);
			cv_images[n] = (cv_images[n] - params.meanValue[0]) / params.stdValue[0];
			memcpy_s(hostDataBuffer + n * height * width, height * width * sizeof(float), cv_images[n].data, height * width * sizeof(float));
		}
	}
	else if (depth == 3) {// ͨ����Ϊ3
		for (int n = 0, index = 0; n < nums; n++)
		{
			cv_images[n].convertTo(cv_images[n], CV_32F, 1.0 / 255);
			std::vector<cv::Mat> bgrChannels(3);
			cv::split(cv_images[n], bgrChannels);
			for (int d = 0; d < 3; d++) {
				bgrChannels[2 - d] = (bgrChannels[2 - d] - params.meanValue[d]) / params.stdValue[d];	// ��ǰͼ��ͨ����RGB��ת��BGR
				memcpy_s(hostDataBuffer + height * width * (3 * n + (2 - d)), height * width * sizeof(float), bgrChannels[2 - d].data, height * width * sizeof(float));
			}
		}
	}
	else {
		LOG_F(INFO, "��֧�ֵ�ͼ������");
		return LY_WRONG_IMG;
	}
	return LY_OK;
}

// \! �쳣������
// \@param buffers: TensorRT samples�ж���Ļ���
// \@param out_masks: �洢���mask��
// \@param engine_output_batch: ONNXģ�͵�batch
// \@param engine_output_height: ONNXģ�͵�height
// \@param engine_output_width: ONNXģ�͵�width
// \@param engine_output_name: ONNXģ�͵��������
int anomalyPost(const samplesCommon::BufferManager & buffers, std::vector<cv::Mat>& out_masks, const int engine_output_batchsize,const int engine_output_height,const int engine_output_width,const std::string outputName)
{
	// ��������
	float* output = static_cast<float*>(buffers.getHostBuffer(outputName));
	// ����out_mask
	for (int i = 0; i < engine_output_batchsize; i++){
			cv::Mat tmp = cv::Mat(
				engine_output_height,
				engine_output_width, 
				CV_32F, 
				output + engine_output_height * engine_output_width * i
			).clone();
			out_masks.push_back(tmp);
	}
	return LY_OK;
}

// \! �Ƚϴ�С
bool PairCompare(const std::pair<float, int>& lhs, const std::pair<float, int>& rhs)
{
	return lhs.first > rhs.first;
}
// \�����������з�������N��÷����,�����С�������Ŀc���������СN
std::vector<int> Argmax(const std::vector<float>& v, int N)
{
	std::vector<std::pair<float, int>> pairs;
	for (size_t i = 0; i < v.size(); ++i)
		pairs.push_back(std::make_pair(v[i], i));

	std::partial_sort(pairs.begin(), pairs.begin() + N, pairs.end(), PairCompare);

	std::vector<int> result;
	for (int i = 0; i < N; ++i)
		result.push_back(pairs[i].second);
	return result;
}
// \!�������
// \@ param buffers:tensorRT sample����Ļ���
// \@ param outputs:������
// \@ param batch:batchsize
// \@ param num_class:�������
// \@ param output_name:onnx�������
int clsPost(const samplesCommon::BufferManager & buffers,std::vector<std::vector<ClassifyResult>>& outputs, const int batch, const int num_class,std::string output_name)
{
	float* output_buffer = static_cast<float*>(buffers.getHostBuffer(output_name));

	// Top K
	int N = 3;
	auto K = N > num_class ? num_class : N;

	for (int b = 0; b < batch; b++) {
		float sum{ 0.0f };
		for (int i = 0; i < num_class; i++) {
			output_buffer[b * num_class + i] = exp(output_buffer[b * num_class + i]);
			sum += output_buffer[b * num_class + i];
		}

		// output���һ��ͼƬ�������������Ŷ�
		std::vector<float> output;
		for (int j = 0; j < num_class; j++) {
			output.push_back(output_buffer[b * num_class + j] / sum);
		}

		// output topk��index ����maxN��
		std::vector<int> maxN = Argmax(output, K);
		std::vector<ClassifyResult> classifyResults;

		for (int i = 0; i < K; ++i)
		{
			int idx = maxN[i];
			classifyResults.push_back(std::make_pair(idx, output[idx]));
		}

		outputs.push_back(classifyResults);
	}
	return LY_OK;
}

// \! �ָ����
// \@param buffers: TensorRT samples�ж���Ļ���
// \@param out_masks: �洢���mask��
// \@param engine_output_batch: ONNXģ�͵�batch
// \@param engine_output_height: ONNXģ�͵�height
// \@param engine_output_width: ONNXģ�͵�width
// \@param engine_output_name: ONNXģ�͵��������
// \@param verbose: ���Ϊtrue,return the probability graph, else return the class id image
int segPost(const samplesCommon::BufferManager & buffers, std::vector<cv::Mat>& out_masks, const int engine_output_batchsize,const int engine_output_height,const int engine_output_width,const std::string outputName,bool verbose=false)
{
	float* output = static_cast<float*>(buffers.getHostBuffer(outputName));
	if (verbose) {
		// TODO
		return LY_OK;
	}
	else {
		// ����out_mask
		for (int i = 0; i < engine_output_batchsize; i++) {
			cv::Mat tmp = cv::Mat(
				engine_output_height,
				engine_output_width,
				CV_32F,
				output + engine_output_height * engine_output_width * i
			).clone();
			out_masks.push_back(tmp);
		}
	}
	return LY_OK;
}
