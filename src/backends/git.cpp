/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: git.cpp
 * Git repository backend
 */


#include <algorithm>
#include <sstream>

#include "options.h"
#include "revision.h"
#include "sys/fs.h"

#include "utils.h"

#include "backends/git.h"


// Constructor
GitBackend::GitBackend(const Options &options)
	: Backend(options)
{

}

// Destructor
GitBackend::~GitBackend()
{

}

// Initializes the backend
void GitBackend::init()
{
	std::string repo = m_opts.repoUrl();
	if (sys::fs::exists(repo + "/HEAD")) {
		setenv("GIT_DIR", repo.c_str(), 1);
	} else if (sys::fs::exists(repo + "/.git/HEAD")) {
		setenv("GIT_DIR", (repo + "/.git").c_str(), 1);
	} else {
		throw PEX(utils::strprintf("Not a git repository: %s", repo.c_str()));
	}
}

// Returns a unique identifier for this repository
std::string GitBackend::uuid()
{
	// TODO
	return utils::join(utils::split(m_opts.repoUrl(), "/"), "_");
}

// Returns the HEAD revision for the current branch
std::string GitBackend::head(const std::string &branch)
{
	return "HEAD";
}

// Returns the standard branch (i.e., master)
std::string GitBackend::mainBranch()
{
	return "master";
}

// Returns a list of available branches
std::vector<std::string> GitBackend::branches()
{
	std::string out = utils::exec("git branch");
	std::vector<std::string> branches = utils::split(out, "\n");
	for (unsigned int i = 0; i < branches.size(); i++) {
		branches[i] = branches[i].substr(2);
	}
	return branches;
}

// Returns a diffstat for the specified revision
Diffstat GitBackend::diffstat(const std::string &id)
{
	std::string out = utils::exec(std::string("git diff -U0 --no-renames -a ")+id+"^!");
	std::istringstream in(out);
	return DiffParser::parse(in);
}

// Returns a revision iterator for the given branch
Backend::RevisionIterator *GitBackend::iterator(const std::string &branch)
{
	std::string out = utils::exec(std::string("git log --format=\"%H\" ")+branch);
	std::vector<std::string> revisions = utils::split(out, "\n");
	if (!revisions.empty()) {
		revisions.pop_back();
	}
	std::reverse(revisions.begin(), revisions.end());
	return new RevisionIterator(revisions);
}

// Returns the revision data for the given ID
Revision *GitBackend::revision(const std::string &id)
{
	std::string meta = utils::exec(std::string("git log -1 --format=\"%ct\n%aN\n%B\" ")+id);
	std::vector<std::string> lines = utils::split(meta, "\n");
	int64_t date;
	std::string author;
	if (!lines.empty()) {
		utils::str2int(lines[0], &date);
		lines.erase(lines.begin());
	}
	if (!lines.empty()) {
		author = lines[0];
		lines.erase(lines.begin());
	}
	std::string msg = utils::join(lines, "\n");
	return new Revision(id, date, author, msg, diffstat(id));
}
