#include <gui/playlist_model.h>
#include <playlist/playlist.h>
#include <elementals.h>
#include <i18n/i18n.h>

static int n_columns(void* data)
{
  return (int) PLAYLIST_MODEL_N_COLUMNS;
}

static GType column_type(void* data, int col)
{
  switch(col) {
    case PLAYLIST_MODEL_COL_NR: 
    case PLAYLIST_MODEL_COL_YEAR: return G_TYPE_INT;
    case PLAYLIST_MODEL_COL_TITLE:
    case PLAYLIST_MODEL_COL_ARTIST:
    case PLAYLIST_MODEL_COL_COMPOSER:
    case PLAYLIST_MODEL_COL_PIECE:
    case PLAYLIST_MODEL_COL_ALBUM_TITLE:
    case PLAYLIST_MODEL_COL_ALBUM_ARTIST:
    case PLAYLIST_MODEL_COL_GENRE:
    case PLAYLIST_MODEL_COL_LENGTH: return G_TYPE_STRING;
    default: return G_TYPE_NONE;
  }
}

static int n_rows(void* data)
{
  playlist_model_t* model = (playlist_model_t*) data;
  if (model->playlist != NULL) {
    return playlist_count(model->playlist);
  } else {
    return 0;
  }
}

static void cell_value(void* data, int row, int col, GValue* value)
{
  playlist_model_t* model = (playlist_model_t*) data;
  
  if (model->playlist == NULL) {
    return;
  }
  
  track_t* t = playlist_get(model->playlist, row);
  switch(col) {
    case PLAYLIST_MODEL_COL_NR: g_value_set_int(value, track_get_nr(t));
    break;
    case PLAYLIST_MODEL_COL_YEAR: g_value_set_int(value, track_get_year(t));
    break;
    case PLAYLIST_MODEL_COL_TITLE: g_value_set_static_string(value, track_get_title(t));
    break;
    case PLAYLIST_MODEL_COL_ARTIST: g_value_set_static_string(value, track_get_artist(t));
    break;
    case PLAYLIST_MODEL_COL_COMPOSER: g_value_set_static_string(value, track_get_composer(t));
    break;
    case PLAYLIST_MODEL_COL_PIECE: g_value_set_static_string(value, track_get_piece(t));
    break;
    case PLAYLIST_MODEL_COL_ALBUM_TITLE: g_value_set_static_string(value, track_get_album_title(t));
    break;
    case PLAYLIST_MODEL_COL_ALBUM_ARTIST: g_value_set_static_string(value, track_get_album_artist(t));
    break;
    case PLAYLIST_MODEL_COL_GENRE: g_value_set_static_string(value, track_get_genre(t));
    break;
    case PLAYLIST_MODEL_COL_LENGTH: {
        long l = track_get_length_in_ms(t);
        int min = l / 1000 / 60;
        int sec = (l / 1000) % 60;
        char s[100];
        sprintf(s,"%02d:%02d",min, sec);
        g_value_take_string(value, strdup(s));
    }
    break;
    default:
    break;
  }
}

const char* i18n_column_name(playlist_column_enum col) {
  switch (col) {
    case PLAYLIST_MODEL_COL_NR: return _("Nr");
    case PLAYLIST_MODEL_COL_YEAR: return _("Year");
    case PLAYLIST_MODEL_COL_TITLE: return _("Title");   
    case PLAYLIST_MODEL_COL_ARTIST: return _("Artist");
    case PLAYLIST_MODEL_COL_COMPOSER: return _("Composer");
    case PLAYLIST_MODEL_COL_PIECE: return _("Piece");
    case PLAYLIST_MODEL_COL_ALBUM_TITLE: return _("Album");
    case PLAYLIST_MODEL_COL_ALBUM_ARTIST: return _("Album artist");
    case PLAYLIST_MODEL_COL_GENRE: return _("Genre");
    case PLAYLIST_MODEL_COL_LENGTH: return _("Length");
    default: return "";
  }
}

const char* column_id(playlist_column_enum col) {
  switch (col) {
    case PLAYLIST_MODEL_COL_NR: return "col_nr";
    case PLAYLIST_MODEL_COL_YEAR: return "col_year";
    case PLAYLIST_MODEL_COL_TITLE: return "col_title";   
    case PLAYLIST_MODEL_COL_ARTIST: return "col_artist";
    case PLAYLIST_MODEL_COL_COMPOSER: return "col_composer";
    case PLAYLIST_MODEL_COL_PIECE: return "col_piece";
    case PLAYLIST_MODEL_COL_ALBUM_TITLE: return "col_album";
    case PLAYLIST_MODEL_COL_ALBUM_ARTIST: return "col_album_artist";
    case PLAYLIST_MODEL_COL_GENRE: return "col_genre";
    case PLAYLIST_MODEL_COL_LENGTH: return "col_length";
    default: return "col_none";
  }
}



static gboolean filter(void* data, int row)
{
  playlist_model_t* model = (playlist_model_t*) data;
  if (model->valid_tracks == NULL) {
    return TRUE;
  } else {
    track_t* trk = playlist_get(model->playlist, row);
    return set_contains(model->valid_tracks, track_get_id_as_str(trk)); 
  }
}

//TODO: Problemen met playlist allocatie!
playlist_model_t* playlist_model_new()
{
  playlist_model_t* model = (playlist_model_t*) mc_malloc(sizeof(playlist_model_t));
  model->playlist = playlist_new(NULL, _("Untitled"));
  model->model = gtk_list_model_new(model, 
                                    n_columns,
                                    column_type,
                                    n_rows,
                                    cell_value
                                    );
  model->valid_tracks = NULL;
  model->col_properties = col_property_hash_new(10, HASH_CASE_SENSITIVE);
  gtk_list_model_set_filter(model->model, filter);
  
  return model;                                    
}

void playlist_model_destroy(playlist_model_t* model)
{
  gtk_list_model_destroy(model->model);
  playlist_destroy(model->playlist);
  col_property_hash_destroy(model->col_properties);
  model->playlist = NULL;
  mc_free(model);
}

GtkTreeModel* playlist_model_gtk_model(playlist_model_t* model)
{
  return GTK_TREE_MODEL(model->model);
}

void playlist_model_set_playlist(playlist_model_t* model, playlist_t* playlist)
{
  playlist_destroy(model->playlist);
  model->playlist = playlist;
  gtk_list_model_refilter(model->model);
}

track_t* playlist_model_get_track(playlist_model_t* model, int row)
{
  return playlist_get(model->playlist, row);
}

static void playlist_assembler(playlist_model_t* model, int row, playlist_t* plp)
{
  track_t* t = playlist_get(model->playlist, row);
  playlist_append(plp, t);
}

playlist_t* playlist_model_get_selected_playlist(playlist_model_t* model)
{
  playlist_t* plp = playlist_new(playlist_get_library(model->playlist),_("Untitled"));
  gtk_list_model_iterate_with_func(model->model,(GtkListModelIterFunc) playlist_assembler, plp);
  return plp;
}

void playlist_model_set_valid(playlist_model_t* model, set_t* valid)
{
  model->valid_tracks = valid;
  gtk_list_model_refilter(model->model);
}


struct data {
  int N;
  long long a;
};

static void iterate_func(playlist_model_t* model, int row, struct data* d)
{
   d->a += str_crc32(track_get_id(playlist_get(model->playlist, row)));
   d->a <<= 1;
   d->N += 1;
}

long long playlist_model_tracks_hash(playlist_model_t* model) 
{
  struct data d = { 0, 0 };
  gtk_list_model_iterate_with_func(model->model,
                                  (GtkListModelIterFunc) iterate_func, 
                                  &d);
  d.a += d.N;
  return d.a;
}

#define CMPI(a,b) ((a < b) ? -1 : ((a == b) ? 0 : 1))
#define CMPTI(n, a, b) CMPI(track_get_##n(a),track_get_##n(b))
#define CMPTS(n, a, b) strcasecmp(track_get_##n(a), track_get_##n(b))

static int cmp(playlist_model_t* model, int row1, int row2)
{
  log_debug3("row1=%d, row2=%d", row1, row2);
  track_t* t1 = playlist_get(model->playlist, row1);
  track_t* t2 = playlist_get(model->playlist, row2);
  
  switch (model->sort_col) { 
    case PLAYLIST_MODEL_COL_NR: return CMPTI(nr, t1, t2);
    case PLAYLIST_MODEL_COL_YEAR: return CMPTI(year, t1, t2);
    case PLAYLIST_MODEL_COL_ARTIST: return CMPTS(artist, t1, t2);
    case PLAYLIST_MODEL_COL_TITLE: return CMPTS(title, t1, t2);
    case PLAYLIST_MODEL_COL_COMPOSER: return CMPTS(composer ,t1, t2); 
    case PLAYLIST_MODEL_COL_PIECE: return CMPTS(piece ,t1, t2);
    case PLAYLIST_MODEL_COL_ALBUM_TITLE: return CMPTS(album_title ,t1, t2);
    case PLAYLIST_MODEL_COL_ALBUM_ARTIST: return CMPTS(album_artist ,t1, t2);
    case PLAYLIST_MODEL_COL_GENRE: return CMPTS(genre ,t1, t2);
    case PLAYLIST_MODEL_COL_LENGTH: return CMPTI(length_in_ms ,t1, t2);
    default: return CMPTI(nr, t1, t2);
  }
}

void playlist_model_sort(playlist_model_t* model, playlist_column_enum e)
{
  model->sort_col = e;
  gtk_list_model_sort(model->model, (int (*)(void*, int, int)) cmp); 
}

/*****************************************************************************
 * Storing column properties
 *****************************************************************************/

el_bool playlist_model_save_col_properties(playlist_model_t* model, const char* filename)
{
  FILE *f = fopen(filename, "wb");
  if (filename != NULL) {
    hash_iter_t iter = col_property_hash_iter(model->col_properties);
    while (!col_property_hash_iter_end(iter)) {
      col_properties_t* p = col_property_hash_iter_data(iter);
      if (!col_properties_write(p, f)) {
        log_error("Error writing col properties");
        fclose(f);
        return el_false;
      }
      iter = col_property_hash_iter_next(iter);       
    }
    fclose(f);
    return el_true;
  } else {
    log_error2("Cannot write file '%s'", filename); 
    return el_false;
  }
  
  // should not be reached
  log_error("this line should not be reached!");
  return el_false;
}

 
static col_properties_t* copy(col_properties_t* p)
{
  col_properties_t* pp = col_properties_new(p->hash);
  int i;
  for(i = 0; i < PLAYLIST_MODEL_N_COLUMNS; ++i) {
    pp->col_index[i] = p->col_index[i];
    pp->col_width[i] = p->col_width[i];
    pp->col_visible[i] = p->col_visible[i];
  }
  return pp;
}

static void destroy(col_properties_t* p)
{
  col_properties_destroy(p);
}
 
IMPLEMENT_HASH(col_property_hash, col_properties_t, copy, destroy);
 
col_properties_t* col_properties_new(long long hash)
{
  int i;
  col_properties_t* p = (col_properties_t*) mc_malloc(sizeof(col_properties_t));
  for(i = 0; i < PLAYLIST_MODEL_N_COLUMNS; ++i) {
    p->col_index[i] = i;
    p->col_width[i] = 100;
    p->col_visible[i] = el_true;
  }
  p->hash = hash;
  char s[1024];
  sprintf(s,"%lld", hash);
  p->hash_as_str = mc_strdup(s);
  return p;
}

void col_properties_destroy(col_properties_t* p)
{
  mc_free(p->hash_as_str);
  mc_free(p);
}

const char* col_properties_key(const col_properties_t* p)
{
  return p->hash_as_str;
}

#define MAGIC_NUMBER 12832
#define VERSION 1

el_bool col_properties_write(const col_properties_t* p, FILE* f)
{
  short magic_number = MAGIC_NUMBER;
  short version = VERSION;
  fwrite(&magic_number, sizeof(magic_number), 1, f);
  fwrite(&version, sizeof(version), 1, f);
  fwrite(p->col_index, sizeof(int), PLAYLIST_MODEL_N_COLUMNS, f);
  fwrite(p->col_width, sizeof(int), PLAYLIST_MODEL_N_COLUMNS, f);
  fwrite(p->col_visible, sizeof(el_bool), PLAYLIST_MODEL_N_COLUMNS, f);
  fwrite(&p->hash, sizeof(long long), 1, f);
  return el_true;
}

el_bool col_properties_read(col_properties_t* p, FILE* f)
{
  short magic_number;
  short version;
  fread(&magic_number, sizeof(magic_number), 1, f);
  fread(&version, sizeof(version), 1, f);
  if (magic_number != MAGIC_NUMBER || version != VERSION) {
    return el_false;
  } else {
    fread(p->col_index, sizeof(int), PLAYLIST_MODEL_N_COLUMNS, f);
    fread(p->col_width, sizeof(int), PLAYLIST_MODEL_N_COLUMNS, f);
    fread(p->col_visible, sizeof(el_bool), PLAYLIST_MODEL_N_COLUMNS, f);
    fread(&p->hash, sizeof(long long), 1, f);
    mc_free(p->hash_as_str);
    char s[1024];
    sprintf(s, "%lld", p->hash);
    p->hash_as_str = mc_strdup(s);
    return el_true;
  }
}

int col_properties_get_width(const col_properties_t* p, int e)
{
  return p->col_width[e];
}

el_bool col_properties_get_visible(const col_properties_t* p, int e)
{
  return p->col_visible[e];
}

int col_properties_get_index(const col_properties_t* p, int e)
{
  return p->col_index[e];
}

void col_properties_set_width(col_properties_t* p, int e, int width)
{
  p->col_width[e] = width;
}

void col_properties_set_visible(col_properties_t* p, int e, el_bool visible)
{
  p->col_visible[e] = visible;
}

void col_properties_set_index(col_properties_t* p, int e, int index)
{
  p->col_index[e] = index;
}

const col_properties_t* playlist_get_col_properties(playlist_model_t* model, long long hash)
{
  char s[1024];
  sprintf(s, "%lld", hash);
  return (const col_properties_t*) col_property_hash_get(model->col_properties, s);
}

void playlist_set_col_properties(playlist_model_t* model, long long hash, const col_properties_t* p)
{
  char s[1024];
  sprintf(s, "%lld", hash);
  col_property_hash_put(model->col_properties, s, (col_properties_t*) p);
}

