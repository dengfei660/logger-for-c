#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

#include "Logger.h"

int main()
{
    LOGF("main","pid:%d,tid:%d",100,200);
    LOGE("main","pid:%d,tid:%d",100,200);
    LOGW("main","pid:%d,tid:%d",100,200);
    LOGI("main","pid:%d,tid:%d",100,200);
    LOGD("main","pid:%d,tid:%d",100,200);
    LOGV("main","pid:%d,tid:%d",100,200);

    return 0;
}