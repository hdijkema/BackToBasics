#include <gui/scanjob.h>

static scanjob_info_t* copy(scanjob_info_t* info)
{
  return info;
}

static void destroy(scanjob_info_t* info)
{
  // does nothing, because this fifo will be
  // read empty
}

IMPLEMENT_FIFO(scanjob_fifo, scanjob_info_t, copy, destroy);


static scan_result_t scan_cb(scan_job_t* job, el_bool ready, 
                             const char* msg, const char* submsg,
                             int n, int of_n)
{
  
  scanjob_info_t* info = (scanjob_info_t*) mc_malloc(sizeof(scanjob_info_t));
  info->ready = ready;
  info->msg = mc_strdup(msg);
  info->submsg = mc_strdup(submsg);
  info->n = n;
  info->of_n = of_n;
  scanjob_fifo_enqueue(job->fifo, info);
    
  return (job->cancelled) ? SCAN_CANCEL : SCAN_CONTINUE;
}

static void* thread_func(void* data)
{
  scan_job_t* job = (scan_job_t*) data;
  job->f(job, scan_cb, job->userdata);  
  return NULL;
}

void scan_job_cancel(GtkButton* button, GtkDialog* dlg)
{
  scan_job_t* job = (scan_job_t*) g_object_get_data(G_OBJECT(dlg), "scanjob");
  job->cancelled = el_true;
}

gboolean scanjob_timeout(gpointer data)
{
  scan_job_t* job = (scan_job_t*) data;
  el_bool ready = el_false;
  
  while (scanjob_fifo_peek(job->fifo) != NULL) {
    scanjob_info_t* info = scanjob_fifo_dequeue(job->fifo);
    
    gtk_label_set_text(job->lbl_scan_msg, info->msg);
    gtk_label_set_text(job->lbl_scan_submsg, info->submsg);
    double prg = ((double) info->n / (double) info->of_n);
    gtk_progress_bar_set_fraction(job->prg_scan, prg);
    
    ready = info->ready;
    
    mc_free(info->msg);
    mc_free(info->submsg);
    mc_free(info);
  }
  
  if (ready) {
    gtk_widget_set_sensitive(GTK_WIDGET(job->btn_scan_ok), TRUE);
    return FALSE;
  } else {
    return TRUE;
  }
}

void run_scan_job_dialog(const char* i18n_title, ScanJobFunction func, GtkBuilder* builder, void* userdata) {
  
  GtkDialog* scandlg = GTK_DIALOG(gtk_builder_get_object(builder, "dlg_scan_generic"));
  GtkLabel* lbl_scan_msg = GTK_LABEL(gtk_builder_get_object(builder, "lbl_scan_msg"));
  GtkLabel* lbl_scan_submsg = GTK_LABEL(gtk_builder_get_object(builder, "lbl_scan_submsg"));
  GtkProgressBar* prg_scan = GTK_PROGRESS_BAR(gtk_builder_get_object(builder, "prg_scan"));
  GtkButton* btn_scan_ok = GTK_BUTTON(gtk_builder_get_object(builder, "btn_scan_ok"));
  
  log_debug2("btn_scan_ok = %p", btn_scan_ok);
  scanjob_fifo* fifo = scanjob_fifo_new();
  
  scan_job_t scanjob = { el_false, 
                         scandlg, lbl_scan_msg, lbl_scan_submsg, prg_scan, btn_scan_ok, 
                         userdata,
                         func, 
                         fifo
                         };
  
  gtk_window_set_title(GTK_WINDOW(scandlg), i18n_title);
  gtk_widget_set_sensitive(GTK_WIDGET(btn_scan_ok), FALSE);
  
  g_object_set_data(G_OBJECT(scandlg), "scanjob", &scanjob);
  
  pthread_t thread;
  int thread_id = pthread_create(&thread, NULL, thread_func, &scanjob);
  
  g_timeout_add(50, scanjob_timeout, &scanjob);
  
  gtk_dialog_run(scandlg);
  
  gtk_widget_hide(GTK_WIDGET(scandlg));
  
  scanjob_fifo_destroy(fifo);
  log_debug2("btn_scan_ok = %p", btn_scan_ok);
}
  
  

