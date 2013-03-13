#ifndef SCANJOB_HOD
#define SCANJOB_HOD

#include <elementals.h>
#include <gtk/gtk.h>

typedef enum {
  SCAN_CONTINUE,
  SCAN_CANCEL
} scan_result_t;

struct __scanjob__;

typedef struct {
  el_bool ready;
  char* msg;
  char* submsg;
  int n;
  int of_n;
} scanjob_info_t; 

DECLARE_FIFO(scanjob_fifo, scanjob_info_t); 

typedef scan_result_t (*ScanJobCBFunc)(struct __scanjob__* job, 
                                       el_bool ready, 
                                       const char* msg, 
                                       const char* submsg, 
                                       int n, 
                                       int of_n);

typedef struct __scanjob__ {
  el_bool cancelled;
  GtkDialog* scandlg;
  GtkLabel* lbl_scan_msg;
  GtkLabel* lbl_scan_submsg;
  GtkProgressBar* prg_scan;
  GtkButton* btn_scan_ok;
  void* userdata;
  void (*f) (struct __scanjob__* job, ScanJobCBFunc func, void* userdata);
  scanjob_fifo* fifo;
} scan_job_t;


typedef void (*ScanJobFunction)(scan_job_t* job, ScanJobCBFunc func, void* userdata);

void run_scan_job_dialog(const char* i18n_title, ScanJobFunction func, GtkBuilder* builder, void* userdata);

#endif
