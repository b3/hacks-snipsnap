/*
 * Export snipsnap data dump into files
 * 
 * Bruno BEAUFILS <bruno@boulgour.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <error.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

/****************************************************************************/

#define DEBUG 0

/****************************************************************************/

static char * snip_root = "snipspace";
static char * user_name = "user";
static char * user_content[] =
  {
    "login",
    "passwd",
    "email",
    "roles",
    "status",
    "cTime",
    "mTime",
    "lastAccess",
    "lastLogin",
    "lastLogout",
    "application",
    NULL
  };
static char * snip_name = "snip";
static char * snip_content[] =
  {
    "name",
    "oUser",
    "cUser",
    "mUser",
    "cTime",
    "mTime",
    "permissions",
    "backlinks",
    "sniplinks",
    "labels",
    "attachments",
    "viewCount",
    "content",
    "application",
    "parentSnip",
    "commentSnip",
    NULL
  };
static char * attachments_name = "attachments";
static char * attachment_name = "attachment";
static char * attachment_content[] =
  {
    "name",
    "content-type",
    "size",
    "date",
    "location",
    "data",
    NULL
  };
static char * command_name;

/****************************************************************************/

static void dmesg(char* format, ...)
{
#if DEBUG > 1
  va_list args;

  va_start(args, format);
  fprintf(stderr, "debug: ");
  vfprintf(stderr, format, args);
  va_end(args);
#endif
}

/****************************************************************************/

static void pmesg(char* format, ...)
{
  va_list args;

  va_start(args, format);
  fprintf(stderr, "%s: ", command_name);
  vfprintf(stderr, format, args);
  if (errno != 0)
  {
    fprintf(stderr, ": %s", strerror(errno));
  }
  fprintf(stderr, "\n");
  va_end(args);
}

/****************************************************************************/

static void usage(char * name)
{
  fprintf(stderr, "\nusage: %s XMLFILE DIR {user|snip}\n\n", name);
}

/****************************************************************************/

static xmlNodePtr next_element(xmlDocPtr doc,
                               xmlNodePtr cur,
                               xmlChar * elt)
{
  if (doc == NULL || cur == NULL || elt == NULL) return NULL;

  cur = cur->next;

  while (cur != NULL && (cur->type != XML_ELEMENT_NODE || xmlStrcmp(cur->name, (const xmlChar*)elt) != 0))
  {
    cur = cur->next;
  }

  return cur;
}

/****************************************************************************/

static xmlNodePtr next_child(xmlDocPtr doc,
                             xmlNodePtr cur,
                             xmlChar * elt)
{
  if (doc == NULL || cur == NULL || elt == NULL) return NULL;

  cur = cur->xmlChildrenNode;

  while (cur != NULL && (cur->type != XML_ELEMENT_NODE || xmlStrcmp(cur->name, (const xmlChar*)elt) != 0))
  {
    cur = cur->next;
  }

  return cur;
}

/****************************************************************************/

static void dump_leaf(xmlDocPtr doc,
                      xmlNodePtr cur,
                      FILE * out)
{
  xmlChar * content;

  if (doc == NULL || cur == NULL) return;

  content = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

  if (content != NULL)
  {
    fprintf(out, "%s", content);
    xmlFree(content);
  }
}

/****************************************************************************/

static char * create_directory(char * dir,
                               int n)
{
  int size; 
  char * name;

  /* Set basename of output directory */
  size = strlen(dir) + 1 + 4 + 1;
  name = (char *) malloc(size);
  if (name == NULL)
  {
    pmesg("cannot allocate memory: file %s: line %d", __FILE__, __LINE__);
    return NULL;
  }
  *name = 0;
  snprintf(name, size, "%s/%.4d", dir, n);
  
  /* Create output directory */
  if (mkdir(name, 0777))
  {
    pmesg("cannot create %s directory", name);
    free(name);
    return NULL;
  }

  return name;
}

/****************************************************************************/

static void dump_element(xmlDocPtr doc,
                         xmlNodePtr cur,
                         char * elt,
                         char * dir)
{
  xmlNodePtr ptr;
  int size;
  char * name;
  FILE * file;

  /* Prepare filename */
  size = strlen(dir) + 1 + xmlStrlen(elt) + 1;
  name = (char *) malloc(size);
  if (name == NULL)
  {
    pmesg("cannot allocate memory: file %s: line %d", __FILE__, __LINE__);  
    return;
  }
  *name = 0;
  snprintf(name, size, "%s/%s", dir, (char *) elt);
  
  /* Open file */
  file = fopen(name, "w");
  if (file == NULL)
  {
    pmesg("cannot open file %s", name);
    free(name);
    return;
  }
  
  /* Write content into file */
  ptr = next_child(doc, cur, elt);
  if (ptr != NULL)
  {
    dump_leaf(doc, ptr, file);
  }
  else
  {
    pmesg("file %s: no element <%s> in <%s>", dir, elt, cur->name);
  }
  
  /* Clean up */
  if (fclose(file) != 0)
  {
    pmesg("cannot close file %s", name);
  }
  free(name);
}

/****************************************************************************/

static int dump_node(xmlDocPtr doc,
                     xmlNodePtr cur,
                     char ** content,
                     char * dir)
{
  int i;

  for (i = 0; content[i] != NULL; i += 1)
  {
    /* dumping attachments ... */
    if (strcmp(content[i], attachments_name) == 0)
    {
      xmlNodePtr ptr;
      int n;
      char * name;

      /* Find first attachment element */
      ptr = next_child(doc, next_child(doc, cur, attachments_name), attachment_name);
      n = 0;

      /* Browse every attachment */
      while (ptr != NULL)
      {
        int e = 0;

        /* Create a directory per attachment... */
        name = create_directory(dir, n);
        if (name == NULL)
        {
          return;
        }

        /* ...and store every attachment infos into it */
        while(attachment_content[e] != NULL)
        {
          dump_element(doc, ptr, attachment_content[e], name);
          e += 1;
        }

        /* Next attachment */
        ptr = next_element(doc, ptr, attachment_name);
        n += 1;
        free(name);
      }
    }
    /* ... or something else*/
    else
    {
      dump_element(doc, cur, content[i], dir);
    }
  }
}

/****************************************************************************/

static int browse_tree(xmlDocPtr doc,
                       xmlNodePtr cur,
                       char * node,
                       char ** content,
                       char * dir)
{
  int n; 

  n = 0;
  while (cur != NULL)
  {
    char * name;
    int size;

    /* Create a directory per node */
    name = create_directory(dir, n);
    if (name == NULL)
    {
      return 1;
    }

    /* Dump content of node */
    if (dump_node(doc, cur, content, name) != 0)
    {
      free(name);
      return -1;
    }
    
    /* Next node */
    cur = next_element(doc, cur, node);
    n += 1;
    free(name);
  }

  return 0;
}

/****************************************************************************/

int check_parameters(int argc,
                     char ** argv)
{
  command_name = argv[0];

  if (argv[1] == NULL)
  {
    pmesg("document filename not specified");
    usage(argv[0]);
    return 1;
  }
  else if (argv[2] == NULL)
  {
    pmesg("output directory path not specified");
    usage(argv[0]);
    return 1;
  }
  else if (argv[3] == NULL)
  {
    pmesg("snip or user not specified");
    usage(argv[0]);
    return 1;
  }

  return 0;
}

/****************************************************************************/

int initialize_tree(char * name,
                    xmlDocPtr * doc,
                    xmlNodePtr * cur)
{
  /* Parse file */
  *doc = xmlParseFile(name);
  if (*doc == NULL )
  {
    pmesg("document not parsed successfully");
    return 1;
  }

  /* Verify tree */
  *cur = xmlDocGetRootElement(*doc);
  if (*cur == NULL) {
    pmesg("empty document");
    xmlFreeDoc(*doc);
    return 1;
  }
  if (xmlStrcmp((*cur)->name, (const xmlChar*) snip_root) != 0)
  {
    pmesg("document of the wrong type, root node != %s", snip_root);
    xmlFreeDoc(*doc);
    return 1;
  }

  return 0;
}

/****************************************************************************/

int initialize_dir(char * name, char ** dir)
{
  *dir = strdup(name);
  if (mkdir(*dir, 0777) != 0)
  {
    pmesg("cannot create %s directory", *dir);
    return 1;   
  }
  
  return 0;
}

/****************************************************************************/

int initialize_search(int argc,
                      char ** argv,
                      char ** node,
                      char *** content)
{
  /* What to export: user or snip ? */
  *node = strdup(argv[3]);
  if (strcmp(*node, user_name) != 0 && strcmp(*node, snip_name) != 0)
  {
    pmesg("cannot extract <%s> element", argv[3]);
    free(*node);
    return 1;       
  }

  /* Extract everything... */
  if (argc - 4 == 0)
  {
    if (strcmp(*node, user_name) == 0)
    {
      *content = user_content;
    }
    else
    {
      *content = snip_content;
    }
  }
  /* ... or specified content only */
  else
  {
    int i;

    *content = (char **) malloc(argc - 4 + 1);
    if (*content == NULL)
    {
      pmesg("cannot allocate memory: file %s: line %d", __FILE__, __LINE__);
      return 1;   
    }

    for (i = 0; argv[i+4] != NULL ; i += 1)
    {
      (*content)[i] = strdup(argv[i+4]);
    }
    (*content)[i] = NULL;
  }
  
  return 0;
}

/****************************************************************************/

int main (int argc, char ** argv)
{
  xmlDocPtr doc;
  xmlNodePtr cur;
  char * node;
  char ** content;
  char * dir;

  /* Initialization */
  if (check_parameters(argc, argv) != 0)
  {
    return 1;
  }
  if (initialize_tree(argv[1], &doc, &cur) != 0)
  {
    return 1;
  }
  if (initialize_dir(argv[2], &dir) != 0)
  {
    xmlFreeDoc(doc);
    return 1;
  }
  if (initialize_search(argc, argv, &node, &content) != 0)
  {
    xmlFreeDoc(doc);
    free(dir);
    return 1;   
  }

  /* Browse tree looking for node beginning at first element under root */
  cur = next_element(doc, cur->xmlChildrenNode, node);
  browse_tree(doc, cur, node, content, dir);

  /* Clean everything */
  xmlFreeDoc(doc);
  free(dir);
  free(node);
  if (argc - 4 != 0)
  {
    int i;

    for (i = 0; content[i] != NULL ; i += 1)
    {
      free(content[i]);
    }
    free(content);
  }

  return 0;
}

/* End */
