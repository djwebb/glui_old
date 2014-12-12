/****************************************************************************

  GLUI User Interface Toolkit
  ---------------------------

     glui_filebrowser.cpp - GLUI_FileBrowser control class


          --------------------------------------------------

  Copyright (c) 1998 Paul Rademacher

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
  claim that you wrote the original software. If you use this software
  in a product, an acknowledgment in the product documentation would be
  appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
  misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.


*****************************************************************************/

#include "GL/glui.h"
#include "glui_internal.h"
#include <sys/types.h>

#ifdef __GNUC__
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include <sys/stat.h>

GLUI_FileBrowser::GLUI_FileBrowser( GLUI_Node *parent,
                                    const char *name,
                                    int type,
                                    int id,
                                    GLUI_CB cb)
{
  common_init();

  set_name( name );
  user_id    = id;
  int_val    = type;
  callback   = cb;

  parent->add_control( this );
  list = new GLUI_List(this, true, 1);
  list->set_object_callback( GLUI_FileBrowser::dir_list_callback, this );
  list->set_click_type(GLUI_DOUBLE_CLICK);
  this->fbreaddir(this->current_dir.c_str());
}

/****************************** GLUI_FileBrowser::draw() **********/

void GLUI_FileBrowser::dir_list_callback(GLUI_Control *glui_object) {
  GLUI_List *list = dynamic_cast<GLUI_List*>(glui_object);
  if (!list)
    return;
  GLUI_FileBrowser* me = dynamic_cast<GLUI_FileBrowser*>(list->associated_object);
  if (!me)
    return;
  int this_item, l;
  const char *selected;
  this_item = list->get_current_item();
  if (this_item > 0) { /* file or directory selected */
    selected = list->get_item_ptr( this_item )->text.c_str();
    l =  strlen(selected) ;
    if (selected[l-1] == '/' || selected[0] == '/' ||
                                selected[0] == '\\') {
      if (me->allow_change_dir) {
#ifdef __GNUC__
        chdir(selected);
#endif
#ifdef _WIN32
        SetCurrentDirectory(selected+1);
#endif
        me->fbreaddir(".");
      }
    } else {
      me->file = selected;
      me->execute_callback();
    }
  }
}

int fb_list_compare(const void *p1, const void *p2)
{
  GLUI_List_Item *lp1 = (GLUI_List_Item *) *(void **)p1,
                 *lp2 = (GLUI_List_Item *) *(void **)p2 ;
  int  l1 = lp1->text.length()+1;
  int  l2 = lp2->text.length()+1;
  int  iret;
  char *str1, *str2 ;

  str1 = (char *)malloc(l1*sizeof(char));
  str2 = (char *)malloc(l2*sizeof(char));
  strncpy(str1, lp1->text.c_str(),l1);
  strncpy(str2, lp2->text.c_str(),l2);

  if(strcmp(str1,"./")==0) iret = true  ;
  else if(strcmp(str2,"./")==0) iret = false ;
  else if(strcmp(str1,"../")==0) iret = true;
  else if(strcmp(str2,"../")==0) iret = false;
  else if((str1[l1-2]=='/') && (str2[l2-2]!='/')) iret = true;
  else if((str1[l1-2]!='/') && (str2[l2-2]=='/')) iret = false;
  else if(strcmp(str1,str2)<0) iret = true;
  else iret = false ;

  free(str1);
  free(str2);
  return !iret ;
}



void GLUI_FileBrowser::fbreaddir(const char *d) {
  GLUI_String item;
  int i = 0;

	if (!d)
    return;

#ifdef _WIN32

  WIN32_FIND_DATA FN;
  HANDLE hFind;
  //char search_arg[MAX_PATH], new_file_path[MAX_PATH];
  //sprintf(search_arg, "%s\\*.*", path_name);

  hFind = FindFirstFile("*.*", &FN);
  if (list) {
    list->delete_all();
    if (hFind != INVALID_HANDLE_VALUE) {
      do {
        int len = strlen(FN.cFileName);
        if (FN.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          item = '\\';
          item += FN.cFileName;
        } else {
          item = FN.cFileName;
        }
        list->add_item(i,item.c_str());
        i++;
      } while (FindNextFile(hFind, &FN) != 0);

      if (GetLastError() == ERROR_NO_MORE_FILES)
        FindClose(hFind);
      else
        perror("fbreaddir");
    }
  }

#elif defined(__GNUC__)

  DIR *dir;
  struct dirent *dirp;
  struct stat dr;
  int iret ;

  if (list) {
    list->delete_all();
    if ((dir = opendir(d)) == NULL)
      perror("fbreaddir:");
    else {
      while ((dirp = readdir(dir)) != NULL)   /* open directory     */
      {
        iret = true ;
        if (!lstat(dirp->d_name,&dr) && S_ISDIR(dr.st_mode)){
          item = dirp->d_name + GLUI_String("/");  /* Directory */
        }else{
          item = dirp->d_name;                     /* File */
          if(list_filter.length()>0){
            if(fnmatch(list_filter.c_str(),item.c_str(),0)!=0) iret = false ;
          }
        }
        if(iret){
          list->add_item(i,item.c_str());
          i++;
        }
      }
      closedir(dir);

/*  Order List */
      {
        int n_list = i;

        GLUI_List_Item **p_list, *p_item;
/* Setup pointers to text and sort */
        p_list = (GLUI_List_Item **)malloc(n_list*sizeof(*p_list));
        for(i=0;i<n_list;i++){ p_list[i] = list->get_item_ptr(i); }
        qsort(p_list,n_list,sizeof(p_list[0]),fb_list_compare);
/* Unlink list */
        for(i=0;i<n_list;i++){ p_list[i]->unlink(); }
/* Relink in new order to parent and siblings */
        p_list[0]->link_this_to_parent_first(&list->items_list);
        for(i=1;i<n_list;i++){
          p_list[i]->link_this_to_sibling_next(p_list[i-1]);
        }
/* Renumber */
        p_item = (GLUI_List_Item *)list->items_list.first_child();
        i = 0;
        while(p_item){
            p_item->id = i++;
            p_item = (GLUI_List_Item *)p_item->next();
        }
        free(p_list);
      }
/* End Order List */
    }
  }
#endif
}

void ProcessFiles(const char *path_name)
{

}

void GLUI_FileBrowser::set_w(int w)
{
  if (list) list->set_w(w);
}

void GLUI_FileBrowser::set_h(int h)
{
  if (list) list->set_h(h);
}

void GLUI_FileBrowser::set_filter(const char *string)
{
    list_filter = string ;
    while(list_filter.length()>0 &&
         (list_filter.at(0)==' ' || list_filter.at(0)=='\t'))list_filter.erase(0,1);
}

const char * GLUI_FileBrowser::filter_cstr()
{
    return list_filter.c_str();
}
