#pragma once


namespace NK
{

	//Empty virtual base struct
	struct BaseComponent
	{
	public:
		BaseComponent() = delete;
		virtual ~BaseComponent() {}
	};

}