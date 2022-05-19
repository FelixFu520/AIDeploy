#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "params.h"
#include "engine.h"
#include "F_log.h"

// ���ࡢ�쳣��⡢�ָ�ǰ����
int normalization(
	std::vector<cv::Mat>& cv_images,//����ͼ��
	std::vector<float> &input_tensor_values,//Ŀ���ַ
	std::shared_ptr<ORTCore_ctx> ctx, //ִ��������
	Ort::MemoryInfo &memory_info	// ort����
);

// \! �������
int clsPost(
	float* floatarr,
	std::vector<std::vector<ClassifyResult>>& outputs,
	const int batch,
	const int num_class
);

// \! �쳣������
int anomalyPost(
	float* floatarr, // onnxruntime ����Ľ��
	std::vector<cv::Mat>& outputs, // �洢����Ľ��
	const int output_batch, // output��batchsize
	const int output_height,//output�ĸ�
	const int output_width//output�Ŀ�
);

// \! �ָ����
int segPost(
	float* floatarr, // onnxruntime ����Ľ��
	std::vector<cv::Mat>& outputs, // �洢����Ľ��
	const int output_batch, // output��batchsize
	const int output_height,//output�ĸ�
	const int output_width//output�Ŀ�
);