/*
 * dll_main.c
 */

#include "dll.h"

#include <proto/exec.h>

#if defined(__AROS__) || defined (__morphos__)
#include <proto/alib.h>
#endif

int main(int argc, char **argv)
{
  if (argc == 2) {
    struct MsgPort *port = CreatePort(argv[1], 0);
    if (port != NULL) {
      int close_request = 0;
      while (close_request == 0) {
        dll_message_t *message;

        /* wait for incoming messages */
        WaitPort(port);

        /* process incoming messages */
        while ((message = (dll_message_t *)GetMsg(port))) {
          switch (message->type) {
            case DLL_MESSAGE_OPEN:
              if ((message->msg.open.cpu != DLL_CURRENT_CPU) || (message->msg.open.os != DLL_CURRENT_OS)) {
                message->result = DLL_RESULT_WRONG_SYSTEM;
              } else {
                message->result = dll_setup();
              }
              if (message->result != DLL_RESULT_OK) {
                close_request = 1; /* give up */
              }
              break;

            case DLL_MESSAGE_GET_SYMBOLS:
              message->msg.get_symbols.symbols = dll_symbols;
              message->result = DLL_RESULT_OK;
              break;

            case DLL_MESSAGE_CLOSE:
              dll_cleanup();
              message->result = DLL_RESULT_OK;
              close_request = 1; /* time to close */
              break;

            default:
              message->result = DLL_RESULT_UNKNOWN_MESSAGE;
              break;
          }
          ReplyMsg((struct Message *)message);
        }
      }
      DeletePort(port);
    }
  }
  return 0;
}
