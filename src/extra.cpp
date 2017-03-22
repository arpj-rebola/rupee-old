#include <stdlib.h>
#include <iostream>
#include <string>

#include "structs.hpp"
#include "extra.hpp"

namespace Parameters {

std::string pathPremise;
std::string pathProof;
int bufferSize = 1000;
int databaseSize = 1000000;
bool verbosity = true;
int hashDepth = 10;
int noVariables = 10000;
bool failFast = true;

void setPremise(std::string path) {
    pathPremise = path;
    Blablabla::log("Premise: " + path);
}

void setProof(std::string path) {
    pathProof = path;
    Blablabla::log("Proof: " + path);
}

void setVerbosity(bool value) {
    verbosity = value;
}

void setBufferSize(int value) {
    Blablabla::log("Buffer size: " + std::to_string(value));
    bufferSize = value;
}

void setDatabaseSize(int value) {
    Blablabla::log("Database size: " + std::to_string(value));
    databaseSize = value;
}

void setHashDepth(int value) {
    Blablabla::log("Hash table depth size: " + std::to_string(value));
    hashDepth = value;
}

void setNoVariables(int value) {
    Blablabla::log("Initial number of variables: " + std::to_string(value));
    noVariables = value;
}

void importNoVariables(parser& p) {
    noVariables = p.maxVariable;
    Blablabla::log("Number of variables: " + std::to_string(noVariables));
}

}




namespace Blablabla {

int level = 0;
int widthcount;
int spacecount;
int* stackit;
long* watchit;
int* clause;

void increase() {
    Blablabla::level++;
}

void decrease() {
    Blablabla::level--;
}

void log(std::string str) {
    if(Parameters::verbosity) {
        for(spacecount = 0; spacecount < level; ++spacecount) {
            std::cout << ":\t";
        }
        std::cout << str << std::endl;
    }
}

void comment(std::string str) {
    std::cout << str << std::endl;
}

std::string litToString(int lit) {
    if(lit == Constants::ReservedLiteral) {
        return "0";
    } else if (lit == -Constants::ReservedLiteral){
        return "-0";
    } else if(lit < 0) {
        std::to_string(lit + Constants::ReservedLiteral);
    } else {
        std::to_string(lit - Constants::ReservedLiteral);
    }
}

void logModel(model& m) {
    if(Parameters::verbosity) {
        for(spacecount = 0; spacecount < level; ++spacecount) {
            std::cout << ":\t";
        }
        widthcount = 0
        stackit = m.start;
        while(stackit < m.used) {
            if(widthcount >= 20) {
                std::cout << std::endl;
                for(spacecount = 0; spacecount < level; ++spacecount) {
                    std::cout << ":\t";
                }
                widthcount = 0;
            }
            if(stackit == m.forced) {
                std::cout << "|| "
            }
            if(stackit == m.head) {
                std::cout << "[" << litToString(*stackit) << "] ";
            } else {
                std::cout << litToString(*stackit) << " ";
            }
            ++widthcount;
        }
        if(stackit == m.forced) {
            std::cout << "|| "
        }
        if(stackit == m.head) {
            std::cout << "[] ";
        }
        std::cout << std::endl;
    }
}

void logWatchList(watchlist& wl, int literal, database& d) {
    if(Parameters::verbosity) {
        log("Clauses in watchlist " + litToString(literal) + ":");
        for(watchit = wl[literal]; *watchit != Constants::EndOfWatchList; ++watchit) {
            log(clauseToString(Database::getPointer(d, *watchit)));
        }
    }
}

void logResolutionCandidates(latency& x, database& d) {
    if(Parameters::verbosity) {
        log("Resolution candidates in database:");
        for(watchit = x.array; *watchit != Constants::EndOfWatchList; ++watchit) {
            log(clauseToString(Database::getPointer(d, *watchit)));
        }
    }
}

void logReasons(model& m, database& d) {
    log("Reason clauses in model:");
    stackit = m.start + 1;
    while(stackit < m.used) {
        log(litToString(*stackit) + "  <-  " + clauseToString(Database::getPointer(m.reasons[*stackit])))
        ++stackit;
    }
}

void logRevision(revision& v) {
    stackit = v.array;
    while(stackit < v.used) {
        std::cout << litToString(*stackit) << " ";
        ++stackit;
    }
    std::cout << std::endl;
}

void logDatabase(database& d) {
	Blablabla::log("Database:");
	Blablabla::increase();
	Blablabla::log("A\tactive");
	Blablabla::log("S\tscheduled");
	Blablabla::log("P\tpremise");
	Blablabla::log("X\tpersistent");
	Blablabla::log("R\treason clause");
    Blablabla::log("C\tconflict clause");
	Blablabla::log("");
	int i = 0;
	bool inclause = false;
	std::string str;
	std::string symbols[6];
	symbols[0] = "A";
	symbols[1] = "S";
	symbols[2] = "P";
	symbols[3] = "X";
	symbols[4] = "R";
    symbols[5] = "C";
	while(i < d.databaseused) {
		if(!inclause) {
			inclause = true;
			str = "";
			for(int j = 0; j <= 5; j++ ) {
				if (((d.databasearray[i] >> j) % 2) != 0) {
					str = str + symbols[j] + " ";
				} else {
					str = str + "  ";
				}
			}
			str = str + "  ";
            str = clauseToString(*(d.databasearray[++i]));
		} else {
			if(d.databasearray[i] == 0) {
                Blablabla::log(str);
				inclause = false;
			}
            ++i;
		}
	}
    Blablabla::log("");
	Blablabla::decrease();
}

void logProof(proof& r, database& d) {
    Blablabla::log("Database:");
	Blablabla::increase();
    for(int i = 0; i < r.used; ++i) {
        if(i == r.noPremises) {
            Blablabla::log("------------------");
            Blablabla::log(instructionToString(r, d, i));
        }
    }
	Blablabla::decrease();
}

std::string clauseToString(int* ptr) {
    std::string ans = "[ ";
    while(*ptr != Constants::EndOfClause) {
        ans = ans + litToString(*ptr) + " ";
        ++ptr;
    }
    ans = ans + "]";
    return ans;
}

std::string instructionToString(proof& r, database& d, int pos) {
    std::string ans = "";
    if(r.kinds[pos] == Constants::InstructionDeletion) {
        ans = "--- [ ";
    } else {
        ans = "+++ [ ";
    }
    clause = Database::getPointer(d, r.array[pos]);
    while(*clause != Constants::EndOfClause) {
        if(*clause == r.pivots[pos]) {
            ans = ans + "*" + litToString(*clause) + " ";
        } else {
            ans = ans + litToString(*clause) + " ";
        }
        ++clause;
    }
    return ans + "]";
}

}
