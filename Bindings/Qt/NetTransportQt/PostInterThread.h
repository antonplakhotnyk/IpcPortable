#pragma once


struct PostInterThread_Imp;

class PostInterThread
{
public:
    
    PostInterThread();
    ~PostInterThread();
    
    void PostEvent();
	void PostEventId(int id);
    
	virtual void OnPostInterThread(){};
	virtual void OnPostInterThreadId(int id){};
    
private:
    
    
    PostInterThread_Imp* m_imp;    
};
