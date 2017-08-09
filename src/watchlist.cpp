#include <stdlib.h>

#include "structs.hpp"
#include "extra.hpp"

namespace WatchList {

//----------------------
// Memory management
//----------------------

int rowit;
bool error;

// Allocates individual watchlist <clarray> with <clmax> slots and sets <clused> to 0.
bool allocate(long*& clarray, int& clused, int& clmax) {
    clarray = (long*) malloc(Parameters::hashDepth * sizeof(long));
    clused = 0;
    clmax = Parameters::hashDepth;
    if(clarray == NULL) {
        return false;
    }
    clarray[0] = Constants::EndOfList;
    return true;
}

// Allocates watchlist <wl>.
bool allocate(watchlist& wl) {
    error = false;
    #ifdef VERBOSE
    Blablabla::log("Allocating watchlist");
    #endif
    wl.array = (long**) malloc((2 * Stats::variableBound + 1) * sizeof(long*));
    wl.used = (int*) malloc((2 * Stats::variableBound + 1) * sizeof(int));
    wl.max = (int*) malloc((2 * Stats::variableBound + 1) * sizeof(int));
    if(wl.array == NULL || wl.used == NULL || wl.max == NULL) {
        error = true;
    } else {
        wl.array += Stats::variableBound;
        wl.used  += Stats::variableBound;
        wl.max   += Stats::variableBound;
        for(rowit = -Stats::variableBound; !error && rowit <= Stats::variableBound; ++rowit) {
            if(!allocate(wl.array[rowit], wl.used[rowit], wl.max[rowit])) {
                error = true;
            }
        }
    }
    if(error) {
        #ifdef VERBOSE
        Blablabla::log("Error at watchlist allocation");
        #endif
        Blablabla::comment("Memory management error");
        return false;
    } else {
        return true;
    }
}

// Reallocates individual watchlist <clarray> with <clmax> slots and <clused> current size.
bool reallocate(long*& clarray, int& clused, int& clmax) {
    #ifdef VERBOSE
    Blablabla::log("Reallocating watchlist");
    #endif
    clmax *= 2;
    clarray = (long*) realloc(clarray, clmax * sizeof(long));
    if(clarray == NULL) {
        #ifdef VERBOSE
        Blablabla::log("Error at watchlist allocation");
        #endif
        Blablabla::comment("Memory management error");
        return false;
    }
    return true;
}

// Deallocates watchlist <wl>.
void deallocate(watchlist& wl) {
    #ifdef VERBOSE
    Blablabla::log("Deallocating watchlist");
    #endif
    for(rowit = -Stats::variableBound; rowit <= Stats::variableBound; ++rowit) {
        free(wl.array[rowit]);
    }
    wl.array -= Stats::variableBound;
    wl.used -= Stats::variableBound;
    wl.max -= Stats::variableBound;
    free(wl.array);
    free(wl.used);
    free(wl.max);
}

//----------------------
// Core commands
//----------------------

int firstlit;
int* firstptr;
int secondlit;
int* secondptr;

bool insertWatches(watchlist &wl, model& m, long offset, int* pointer) {
    if(Database::isEmpty(pointer)) {
        #ifdef VERBOSE
        Blablabla::log("Empty clause, no watches inserted");
        #endif
        if(!Model::isContradicted(m)) {
            Model::propagateLiteral(m, Constants::ConflictLiteral, offset, pointer, Constants::HardPropagation);
        }
    } else {
        firstptr = pointer;
        if(Database::nextNonFalsified(firstptr, firstlit, m)) {
            *firstptr = *pointer;
            *pointer = firstlit;
            secondptr = firstptr + 1;
            secondlit = pointer[1];
            if(!WatchList::addWatch(wl, firstlit, offset)) { return false; }
            if(!Model::isSatisfied(m, firstlit)) {
                if(Database::nextNonFalsified(secondptr, secondlit, m)) {
                    *secondptr = pointer[1];
                    pointer[1] = secondlit;
                } else {
                    Model::propagateLiteral(m, firstlit, offset, pointer, Constants::HardPropagation);
                }
            }
            if(!WatchList::addWatch(wl, secondlit, offset));
            #ifdef VERBOSE
            Blablabla::log("Setting watches: " + Blablabla::clauseToString(pointer));
            #endif
        } else {
            if(!WatchList::addWatch(wl, pointer[0], offset)) { return false; }
            if(!WatchList::addWatch(wl, pointer[1], offset)) { return false; }
            #ifdef VERBOSE
            Blablabla::log("Setting watches: " + Blablabla::clauseToString(pointer));
            #endif
            Model::propagateLiteral(m, Constants::ConflictLiteral, offset, pointer, Constants::HardPropagation);
        }
    }
    return true;
}

void removeWatches(watchlist &wl, long offset, int* pointer) {
    if(!Database::isEmpty(pointer)) {
        findAndRemoveWatch(wl, pointer[0], offset);
        findAndRemoveWatch(wl, pointer[1], offset);
    }
}

bool alignWatches(watchlist& wl, model& m, long offset, int* pointer, int literal, long*& watch, bool hard) {
    #ifdef VERBOSE
    Blablabla::log("Aligning clause " + Blablabla::clauseToString(pointer));
    Blablabla::increase();
    #endif
    if(Model::isSatisfied(m, pointer[0]) || Model::isSatisfied(m, pointer[1])) {
        #ifdef VERBOSE
        Blablabla::log("Watches are correct");
        #endif
        ++watch;
    } else {
        if(pointer[0] == literal) {
            pointer[0] = pointer[1];
        }
        firstptr = pointer + 2;
        if(Database::nextNonFalsified(firstptr, firstlit, m)) {
            *firstptr = literal;
            pointer[1] = firstlit;
            #ifdef VERBOSE
            Blablabla::log("Setting watches: " + Blablabla::clauseToString(pointer));
            #endif
            removeWatch(wl, literal, watch);
            if(!addWatch(wl, firstlit, offset)) { return false; }
        } else {
            pointer[1] = literal;
            #ifdef VERBOSE
            Blablabla::log("Triggered clause");
            #endif
            if(Model::isFalsified(m, pointer[0])) {
                Model::propagateLiteral(m, Constants::ConflictLiteral, offset, pointer, hard);
            } else {
                Model::propagateLiteral(m, *pointer, offset, pointer, hard);
            }
            ++watch;
        }
    }
    #ifdef VERBOSE
    Blablabla::decrease();
    #endif
    return true;
}

bool reviseWatches(watchlist& wl, model& m, long offset, int* pointer, int literal, long*& watch) {
    #ifdef VERBOSE
    Blablabla::log("Revising clause " + Blablabla::clauseToString(pointer));
    Blablabla::increase();
    #endif
    if(pointer[1] == literal) {
        pointer[1] = pointer[0];
        pointer[0] = literal;
    }
    if(Model::isFalsified(m, pointer[1])) {
        firstptr = pointer + 2;
        if(Database::nextNonFalsified(firstptr, firstlit, m)) {
            *firstptr = pointer[1];
            pointer[1] = firstlit;
            removeWatch(wl, literal, watch);
            if(!addWatch(wl, firstlit, offset)) { return false; }
            #ifdef VERBOSE
            Blablabla::log("Setting watches: " + Blablabla::clauseToString(pointer));
            #endif
        } else {
            #ifdef VERBOSE
            Blablabla::log("Triggered clause");
            #endif
            Model::propagateLiteral(m, literal, offset, pointer, Constants::HardPropagation);
        }
    } else {
        #ifdef VERBOSE
        Blablabla::log("Watches are correct");
        #endif
        ++watch;
    }
    #ifdef VERBOSE
    Blablabla::decrease();
    #endif
    return true;
}

//----------------------
// List processing
//----------------------

long* listwatch;
long listoffset;
int* listclause;


bool alignList(watchlist& wl, model& m, database& d, int literal, bool hard) {
    #ifdef VERBOSE
    Blablabla::log("Aligning watchlist for literal " + Blablabla::litToString(literal));
    Blablabla::increase();
    Blablabla::logWatchList(wl, literal, d);
    #endif
    listwatch = wl.array[literal];
    while((listoffset = *listwatch) != Constants::EndOfList && !Model::isContradicted(m)) {
        listclause = Database::getPointer(d, listoffset);
        if(!alignWatches(wl, m, listoffset, listclause, literal, listwatch, hard)) { return false; }
    }
    #ifdef VERBOSE
    Blablabla::decrease();
    #endif
    return true;
}

bool reviseList(watchlist& wl, model& m, database& d, int literal) {
    #ifdef VERBOSE
    Blablabla::log("Revising watchlist for literal " + Blablabla::litToString(literal));
    Blablabla::increase();
    Blablabla::logWatchList(wl, literal, d);
    #endif
    listwatch = wl.array[literal];
    while((listoffset = *listwatch) != Constants::EndOfList && !Model::isSatisfied(m, literal)) {
        listclause = Database::getPointer(d, listoffset);
        if(!reviseWatches(wl, m, listoffset, listclause, literal, listwatch)) { return false; }
    }
    #ifdef VERBOSE
    Blablabla::decrease();
    #endif
    return true;
}

bool collectList(watchlist& wl, latency& lt, database& d, int pivot, int literal) {
    listwatch = wl.array[literal];
    while((listoffset = *(listwatch++)) != Constants::EndOfList) {
        listclause = Database::getPointer(d, listoffset);
        if(listclause[0] == literal && Database::isFlag(listclause, Constants::VerificationBit, Constants::ScheduledFlag) && Database::containsLiteral(listclause, pivot)) {
            if(!Latency::addCandidate(lt, listoffset)) { return false; }
        }
    }
    return true;
}

//----------------------
// Watchlist handling
//----------------------

long* removewatch;
long removeoffset;

// Adds <offset> at the end of <wl[literal]>
bool addWatch(watchlist &wl, int literal, long offset) {
    if(wl.used[literal] >= wl.max[literal] - 1) {
        if(!reallocate(wl.array[literal], wl.used[literal], wl.max[literal])) { return false; }
    }
    wl.array[literal][wl.used[literal]++] = offset;
    wl.array[literal][wl.used[literal]] = Constants::EndOfList;
    return true;
}
//
// // Removes the offset pointed by <watch> from <wl[literal]>
// // Behavior is undefined if <watch> does not point to an element in <wl[literal]>
void removeWatch(watchlist &wl, int literal, long* watch) {
    *watch = wl.array[literal][--wl.used[literal]];
    wl.array[literal][wl.used[literal]] = Constants::EndOfList;
}
//
// // Removes the first occurrence of <offset> in <wl[literal]>, if there is one;
// // otherwise it does nothing.
void findAndRemoveWatch(watchlist& wl, int literal, long offset) {
    removewatch = wl.array[literal];
    while((removeoffset = *removewatch) != Constants::EndOfList) {
        if(removeoffset == offset) {
            removeWatch(wl, literal, removewatch);
            return;
        } else {
            ++removewatch;
        }
    }
}

}
