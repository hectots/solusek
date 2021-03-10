/*
** Solusek by Alessandro Ubriaco
**
** Copyright (c) 2019 Alessandro Ubriaco
**
*/
#include "CTransaction.h"
#include "../include/solusek/string_util.h"
#include "CDatabaseHandle.h"
#include <unistd.h>

namespace solusek
{
	bool CTransaction::Lock = false;


	CTransaction::~CTransaction()
	{
		cleanup();
		delete T;
	}

	MDatabaseHandle *CTransaction::insert(const char *tableName, std::map<std::string, std::string> vars)
	{
		CDatabaseHandler *h = (CDatabaseHandler*)H;
		std::string acc(""), vals("");
		for(std::map<std::string,std::string>::iterator it = vars.begin(); it != vars.end(); ++it)
		{
			if(acc.size() > 0)
				acc += ",";
			if(vals.size() > 0)
				vals += ",";
			acc += it->first;
			vals += "'";
			vals += esc(it->second);
			vals += "'";
		}

		std::string builder("INSERT INTO ");
		builder += tableName;
		builder += " (";
		builder += acc;
		builder += ") VALUES (";
		builder += vals;
		builder += ")";

		while(Lock)
			usleep(1);
		Lock = true;
		MDatabaseHandle *r = new CDatabaseHandle(T->insert(builder, "id"));
		Lock = false;
		Handles.push_back(r);
		return r;
	}

	bool CTransaction::update(const char *tableName, std::map<std::string, std::string> find, std::map<std::string, std::string> vars)
	{
		CDatabaseHandler *h = (CDatabaseHandler*)H;
		std::string acc(""), vals("");
		for(std::map<std::string,std::string>::iterator it = find.begin(); it != find.end(); ++it)
		{
			if(acc.size() > 0)
				acc += " AND ";
			acc += it->first;
			acc += "='";
			acc += it->second;
			acc += "'";
		}
		for(std::map<std::string,std::string>::iterator it = vars.begin(); it != vars.end(); ++it)
		{
			if(vals.size() > 0)
				vals += ",";
			vals += it->first;
			vals += "='";
			vals += esc(it->second);
			vals += "'";
		}

		std::string builder("UPDATE ");
		builder += tableName;
		builder += " SET ";
		builder += vals;
		builder += " WHERE ";
		builder += acc;

		while(Lock)
			usleep(1);
		Lock = true;
		bool r = T->query(builder,0);
		Lock = false;
		return r;
	}

	MDatabaseHandle *CTransaction::selectOne(const char *tableName, std::vector<std::string> find, std::map<std::string, std::string> vars, const std::string &sort)
	{
		std::string acc(""), q("");
		for(std::vector<std::string>::iterator it = find.begin(); it != find.end(); ++it)
		{
			if(q.size() > 0)
				q += ",";
			q += (*it);
		}
		for(std::map<std::string,std::string>::iterator it = vars.begin(); it != vars.end(); ++it)
		{
			if(acc.size() > 0)
				acc += " AND ";
			acc += it->first;
			acc += "='";
			acc += esc(it->second);
			acc += "'";
		}

		std::string builder("SELECT " + q + " FROM ");
		builder += tableName;
		if(vars.size() > 0)
		{
			builder += " WHERE ";
			builder += acc;
		}
		if(sort.size() > 0)
		{
			builder += " ORDER BY ";
			builder += sort;
		}
		builder += " LIMIT 1";

		while(Lock)
			usleep(1);
		Lock = true;
		MDatabaseHandle *r = new CDatabaseHandle(T->queryOne(builder));
		Lock = false;
		Handles.push_back(r);
		return r;
	}

	MDatabaseHandle *CTransaction::selectCustom(const std::string &query)
	{
		CDatabaseResource* rs = new CDatabaseResource(0, "");

		while(Lock)
			usleep(1);
		Lock = true;
		T->query(query, selectCallback, rs);
		Lock = false;

		MDatabaseHandle *r = new CDatabaseHandle(rs);
		Handles.push_back(r);
		return r;
	}

	MDatabaseHandle *CTransaction::select(const char *tableName, std::vector<std::string> find, std::map<std::string, std::string> vars, const std::string &sort, const std::string &limit)
	{
		std::string acc(""), q("");
		for(std::vector<std::string>::iterator it = find.begin(); it != find.end(); ++it)
		{
			if(q.size() > 0)
				q += ",";
			q += (*it);
		}
		for(std::map<std::string,std::string>::iterator it = vars.begin(); it != vars.end(); ++it)
		{
			if(acc.size() > 0)
				acc += " AND ";
			acc += it->first;
			acc += "='";
			acc += esc(it->second);
			acc += "'";
		}

		std::string builder("SELECT " + q + " FROM ");
		builder += tableName;
		if(vars.size() > 0)
		{
			builder += " WHERE ";
			builder += acc;
		}
		if(sort.size() > 0)
		{
			builder += " ORDER BY ";
			builder += sort;
		}
		if(limit.size() > 0)
		{
			builder += " LIMIT ";
			builder += limit;
		}


		CDatabaseResource* rs = new CDatabaseResource(0, "");
		while(Lock)
			usleep(1);
		Lock = true;
		T->query(builder, selectCallback, rs);
		Lock = false;
		MDatabaseHandle *r = new CDatabaseHandle(rs);
		Handles.push_back(r);
		return r;
	}

	void CTransaction::selectCallback(const DBROW &row, void *rs)
	{
		CDatabaseResource *res = (CDatabaseResource*)rs;
		std::vector<std::string> r;
		for(unsigned int n = 0; n < row.size(); n++)
		{
			r.push_back(row[n]);
		}
		res->Data.push_back(r);
	}

	void CTransaction::dispose()
	{
		delete this;
	}

	void CTransaction::commit()
	{
		//T->commit();
		while(Lock)
			usleep(1);
		Lock = true;
		T->exec("commit;");
		Lock = false;
	}

	void CTransaction::cleanup()
	{
		for(std::vector<MDatabaseHandle*>::iterator it = Handles.begin(); it != Handles.end(); ++it)
		{
			delete (CDatabaseHandle*)(*it);
		}
		Handles.clear();
	}

	std::string CTransaction::esc(const std::string esc)
	{
		while(Lock)
			usleep(1);
		Lock = true;
		return T->esc(esc);
		Lock = false;
	}
}
