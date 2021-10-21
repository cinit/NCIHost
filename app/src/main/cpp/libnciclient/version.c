#ifndef NCI_HOST_VERSION
#error Please define macro NCI_HOST_VERSION in CMakeList.txt
#endif

__attribute__((used, section("NCI_HOST_VERSION"), visibility("default")))
const char g_nci_host_version[] = NCI_HOST_VERSION;
