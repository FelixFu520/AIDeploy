/*****************************************************************************
* @author : FelixFu
* @date : 2021/10/10 14:40
* @last change :
* @description : TensorRT Engine
*****************************************************************************/
#pragma once
#include <queue>	// ϵͳ��

#include <cuda_runtime.h>  // cuda��
#include <cuda_runtime_api.h>

#include <opencv2/opencv.hpp> // opencv��

#include "NvInfer.h"  //TensorRT��
#include "NvOnnxParser.h"

#include "common.h"  // TensorRT samples�еĺ���
#include "buffers.h"
#include "logging.h"

#include "params.h"	// �Զ�������ݽṹ�� ����Ľӿ�
#include "loguru.hpp" // https://github.com/emilk/loguru


// \! ����ָ��, ��������TRT�����м����͵Ķ�ռ����ָ��
template <typename T>
using SampleUniquePtr = std::unique_ptr<T, samplesCommon::InferDeleter>;



// \! ------------------------------------Engine Start------------------------------
class TRTEngine {
public:
	TRTEngine() {};

	// \!���湹��
	// \@param ����Params����
	// \@param �������nErrorFlag
	TRTEngine(const Params& params, int& nErrnoFlag); 

	std::shared_ptr<nvinfer1::ICudaEngine> Get() const;

public:
	// \! �����������
	std::vector<nvinfer1::Dims> mInputDims;	                    // The dimensions of the input to the network.
	std::vector<nvinfer1::Dims> mOutputDims;                     // The dimensions of the output to the network.
	std::vector<std::string> mInputTensorNames;	    // ģ�����������
	std::vector<std::string> mOutputTensorNames;	// ģ�����������

private:
	int buildONNX(const Params& params);			// Build Engine
	int loadEngine(const Params& params);			// ����Engine

private:
	std::shared_ptr<nvinfer1::ICudaEngine> mEngine; //!< The TensorRT engine used to run the network

};
// \! ------------------------------------Engine End------------------------------


// \! ----------------------Context ִ���������̳߳ز���----------------------------

// \! A simple threadsafe queue using a mutex and a condition variable.
template <typename T>
class Queue
{
public:
	Queue() = default;

	Queue(Queue&& other)
	{
		std::unique_lock<std::mutex> lock(other.mutex_);
		queue_ = std::move(other.queue_);
	}

	void Push(T value)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		queue_.push(std::move(value));
		lock.unlock();
		cond_.notify_one();
	}

	T Pop()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] {return !queue_.empty(); });
		T value = std::move(queue_.front());
		queue_.pop();
		return value;
	}

	size_t Size()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		return queue_.size();
	}

private:
	mutable std::mutex mutex_;
	std::queue<T> queue_;
	std::condition_variable cond_;
};

// \! A pool of available contexts is simply implemented as a queue for our example. 
template <typename Context>
using ContextPool = Queue<std::unique_ptr<Context>>;

// \! A RAII class for acquiring an execution context from a context pool. 
template <typename Context>
class ScopedContext
{
public:
	explicit ScopedContext(ContextPool<Context>& pool)
		: pool_(pool), context_(pool_.Pop())
	{
		context_->Activate();
	}

	~ScopedContext()
	{
		context_->Deactivate();
		pool_.Push(std::move(context_));
	}

	Context* operator->() const
	{
		return context_.get();
	}

private:
	ContextPool<Context>& pool_;
	std::unique_ptr<Context> context_;
};

// \! ִ��������
class ExecContext
{
public:
	friend ScopedContext<ExecContext>;

	ExecContext(int device, std::shared_ptr<nvinfer1::ICudaEngine> engine) : device_(device)
	{
		cudaError_t st = cudaSetDevice(device_);

		if (st != cudaSuccess)
			throw std::invalid_argument("could not set CUDA device");

		context_.reset(engine->createExecutionContext());
	}

	nvinfer1::IExecutionContext* getContext()
	{
		return context_.get();
	}

private:
	void Activate() {}
	void Deactivate() {}
private:
	int device_;
	SampleUniquePtr<nvinfer1::IExecutionContext> context_;
};
// \! ----------------------Context ִ���������̳߳ز��� END---------------------------


// \! ִ�������Ľṹ��
struct TRTCore_ctx
{
	Params params;							// \! ִ�������ĵ�Engine
	std::shared_ptr<TRTEngine> engine;		// \! һ��ģ�Ͷ�Ӧһ��Engine
	ContextPool<ExecContext> pool;          // \! һ��Engine ��Ӧ���ģ��
};

