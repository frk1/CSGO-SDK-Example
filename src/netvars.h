#pragma once

#include "sdk.h"

class CNetVars
{
public:
	CNetVars(void);
	int GetOffset(const char *tableName, const char *propName);
	bool HookProp(const char *tableName, const char *propName, RecvVarProxyFn fun);
	void DumpNetvars();

private:
	int GetProp(const char *tableName, const char *propName, RecvProp **prop = 0);
	int GetProp(RecvTable *recvTable, const char *propName, RecvProp **prop = 0);
	RecvTable *GetTable(const char *tableName);
	std::vector<RecvTable*>    m_tables;
	void DumpTable(RecvTable *table);
};

extern CNetVars *g_pNetVars;