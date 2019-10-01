#include <jni.h>
#include <string>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <stdint.h> // uintptr_t

#include "MemoryManager.h"

#define OUT_PATH "mnt/ubuntu/home/reptilian/logs/"

#define WORD_SIZE 2

void jthrow(JNIEnv* env, const char* cls, const char* msg) {
  env->ThrowNew(env->FindClass(cls), msg);
}

// Provided example native call
extern "C"
JNIEXPORT jstring JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_stringFromJNI(JNIEnv* env, jobject) {
  return env->NewStringUTF("Hello from C++");
}

static MemoryManager* mm;
static size_t sizeInWords = 0;

extern "C"
JNIEXPORT void JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_initMemoryManager(JNIEnv* env, jobject, jint maxAllocationSize) {
  sizeInWords = maxAllocationSize;
  mm = new MemoryManager(WORD_SIZE, bestFit);
  mm->initialize(sizeInWords);
}

extern "C"
JNIEXPORT void JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_deleteMemoryManager(JNIEnv* env, jobject){
  delete mm;
  mm = nullptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_shutdown(JNIEnv* env, jobject){
  mm->shutdown();
  mm->initialize(sizeInWords);
}

extern "C"
JNIEXPORT jint JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_getFragSize(JNIEnv* env, jobject){
  return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_getFreeSize(JNIEnv* env, jobject) {
  return mm->getFreeSize();
}

extern "C"
JNIEXPORT jint JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_getUseSize(JNIEnv* env, jobject) {
  return mm->getUseSize();
}

extern "C"
JNIEXPORT jstring JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_allocateMemory(JNIEnv* env, jobject, jint size){
  void* p = mm->allocate(size * mm->getWordSize());
  if (p) {
    std::stringstream ss;
    ss << p;
    std::string s = ss.str();
    return env->NewStringUTF(s.c_str());
  }
  else {
    return env->NewStringUTF("RIP");
  }
}

extern "C"
JNIEXPORT void JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_freeMemory(JNIEnv* env, jobject, jstring js) {
  const char* cs = env->GetStringUTFChars(js, nullptr);
  std::stringstream ss(cs);
  env->ReleaseStringUTFChars(js, cs);
  void* p;
  ss >> p;
  if (ss.fail() || ! ss.eof()) jthrow(env, "java/lang/IllegalArgumentException", "invalid address");
  mm->free(p);
}

extern "C"
JNIEXPORT void JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_setAlgorithm(JNIEnv* env, jobject, jint alg) {
  if (alg == 1) mm->setAllocator(bestFit);
  else if (alg == 2) mm->setAllocator(worstFit);
  else jthrow(env, "java/lang/IllegalArgumentException", "algorithm must be 1 or 2");
}

extern "C"
JNIEXPORT void JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_writeLogs(JNIEnv* env, jobject){
  mm->dumpMemoryMap(OUT_PATH);
}




