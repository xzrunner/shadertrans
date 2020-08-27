#pragma once

namespace shadertrans
{

class GLSLangAdapter
{
public:
	void Init();

	static GLSLangAdapter* Instance();

private:
	GLSLangAdapter();
	~GLSLangAdapter();

private:
	bool m_inited = false;

	static GLSLangAdapter* m_instance;

}; // GLSLangAdapter

}