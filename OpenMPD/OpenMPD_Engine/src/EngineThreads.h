#pragma once

	void* engineWritter(void* arg);
	void* engineReader(void* arg);

	class EngineWriterListener {
	public: 
		virtual void startCycle() = 0;
		//virtual void dataReady(cl_event dataReadyEvent) = 0;
		//virtual void postRender() = 0;
		virtual void waitNextCycle() = 0;
	};

	void addEngineWriterListener(EngineWriterListener* e);
	void removeEngineWriterListener(EngineWriterListener* e);
