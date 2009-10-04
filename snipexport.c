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

#define DEBUG 1

/****************************************************************************/

static char * snip_root = "snipspace";
static char * attachments_name = "attachments";
static char * attachment_name = "attachment";
static char * attachment_content[] = {
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

static void dmesg(char* format, ...);
static void pmesg(char* format, ...);
static void usage(char * name);
static xmlNodePtr next_element(xmlDocPtr doc, xmlNodePtr cur, xmlChar * elt);
static xmlNodePtr next_child(xmlDocPtr doc, xmlNodePtr cur, xmlChar * elt);
static void show_element(xmlDocPtr doc, xmlNodePtr cur, FILE * out);
static void write_element(xmlDocPtr doc, xmlNodePtr cur, xmlChar * elt, char * dir, char * name);
static void write_attachment(xmlDocPtr doc, xmlNodePtr cur, xmlChar * elt, char * dir, char * name, int n);

/****************************************************************************/

static void dmesg(char* format, ...)
{
#ifdef DEBUG
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
  fprintf(stderr, "\nusage: %s XMLFILE OUTPUT_DIR 1ST_LEVEL_ELT CONTENT_ELT\n\n", name);
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

static void show_element(xmlDocPtr doc,
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

static void write_element(xmlDocPtr doc,
                          xmlNodePtr cur,
                          xmlChar * elt,
                          char * dir,
                          char * name)
{
  xmlNodePtr ptr;
  int size;
  char * fname;
  FILE * file;

  /* Prepare filename */
  size = strlen(dir) + 1 + strlen(name) + 1 + xmlStrlen(elt) + 1;
  fname = (char *) malloc(size);
  if (fname == NULL)
  {
    pmesg("cannot allocate memory: file %s: line %d", __FILE__, __LINE__);
  }
  *fname = 0;
  snprintf(fname, size, "%s/%s-%s", dir, name, (char *) elt);
  
  /* Open file */
  file = fopen(fname, "w");
  if (file == NULL)
  {
    pmesg("cannot open file %s", fname);
    free(fname);
    return;
  }
  
  /* Get content into file */
  ptr = next_child(doc, cur, elt);
  if (ptr != NULL)
  {
    show_element(doc, ptr, file);
  }
  else
  {
    pmesg("no element <%s> in <%s> for %s", elt, cur->name, name);
  }
  
  /* Clean up */
  if (fclose(file) != 0)
  {
    pmesg("cannot close file %s", fname);
  }
  free(fname);
}

/****************************************************************************/

static void write_attachment(xmlDocPtr doc,
                             xmlNodePtr cur,
                             xmlChar * elt,
                             char * dir,
                             char * name,
                             int n)
{
  char * fname;
  int size;

  /* Prepare output file name */
  size = strlen(name) + 1 + strlen(attachment_name) + 1 + 4 + 1;
  fname = (char *) malloc(size);
  if (fname == NULL)
  {
    pmesg("cannot allocate memory: file %s: line %d", __FILE__, __LINE__);
  }
  *fname = 0;
  snprintf(fname, size, "%s-%s-%.4d", name, attachment_name, n);

  /* Get the attachment */
  write_element(doc, cur, elt, dir, fname);

  /* Clean up */
  free(fname);
}

/****************************************************************************/

int main (int argc, char ** argv)
{
  xmlDocPtr doc;
  xmlNodePtr cur;
  xmlChar * name;
  xmlChar * searched_node;
  xmlChar ** searched_content;
  char * output_dir;
  int content_size;
  int n;
  int i;

  command_name = argv[0];

  /* Checking parameters */
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
    pmesg("first level element not specified");
    usage(argv[0]);
    return 1;
  }
  else if (argv[4] == NULL)
  {
    pmesg("content to dump not specified");
    usage(argv[0]);
    return 1;
  }

  /* Parse file */
  doc = xmlParseFile(argv[1]);
  if (doc == NULL ) {
    pmesg("document not parsed successfully");
    return 1;
  }

  cur = xmlDocGetRootElement(doc);
  if (cur == NULL) {
    pmesg("empty document");
    xmlFreeDoc(doc);
    return 1;
  }

  if (xmlStrcmp(cur->name, (const xmlChar*) snip_root) != 0)
  {
    pmesg("document of the wrong type, root node != %s", snip_root);
    xmlFreeDoc(doc);
    return 1;
  }

  /* Output directory */
  output_dir = strdup(argv[2]);
  if (mkdir(output_dir, 0777) != 0)
  {
    pmesg("cannot create output directory");
    xmlFreeDoc(doc);
    return 1;   
  }

  /* First level element name */
  searched_node = xmlCharStrdup(argv[3]);

  /* List of contents to get in each first level element */
  content_size = argc - 4;

  searched_content = (xmlChar **) malloc(content_size * sizeof(xmlChar *));
  if (searched_content == NULL)
  {
    pmesg("cannot allocate memory: file %s: line %d", __FILE__, __LINE__);
    xmlFreeDoc(doc);
    return 1;   
  }

  for (i = 0; i < content_size ; i += 1)
  {
    searched_content[i] = xmlCharStrdup(argv[i+4]);
  }

  /* First element under root element */
  cur = next_element(doc, cur->xmlChildrenNode, searched_node);

  /* Browse tree for each element */
  n = 0;
  do
  {
    char name[5];
    int i;

    /* Fix base name of output file */
    *name = 0;
    snprintf(name, 5, "%.4d", n);

    /* Browse asked content */
    for (i = 0; i < content_size; i += 1)
    {
      /* Any data other than attachments are identically stored */
      if (strcmp(searched_content[i], attachments_name) != 0)
      {
        write_element(doc, cur, searched_content[i], output_dir, name);
      }
      /* Attachments are special and may be multiple */
      else
      {
        xmlNodePtr ptr;
        int an;

        ptr = next_child(doc, next_child(doc, cur, attachments_name), attachment_name);
        an = 0;

        while (ptr != NULL)
        {
          int e = 0;

          while(attachment_content[e] != NULL)
          {
            write_attachment(doc, ptr, attachment_content[e], output_dir, name, an);
            e += 1;
          }
          ptr = next_element(doc, ptr, attachment_name);
          an += 1;
        }
      }
    }
    
    /* Next element */
    cur = next_element(doc, cur, searched_node);
    n += 1;
  }
  while (cur != NULL);

  /* Clean everything */
  xmlFreeDoc(doc);
  free(output_dir);
  free(searched_node);
  for (i = 0; i < content_size ; i += 1)
  {
    free(searched_content[i]);
  }
  free(searched_content);

  return 0;
}
