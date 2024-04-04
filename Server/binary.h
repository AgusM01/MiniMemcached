#ifndef __BINARY_H__
#define __BINARY_H__

/*
 * Consume el binario del fd de un cliente.
 * Realiza operaciones que previenen que no llegue la totatildad del mensaje 
*/
int binary_consume(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist);

/*Leo la longitud del dato o la clave*/
int read_length(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist);

/*Leo la clave o el dato basandome en la longitud que previamente calculé.*/
int read_content(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist, int data_or_key);

/*
 * Reinicio los valores cuando termino de responder un pedido correctamente. 
 * No puedo reiniciar antes ya que quizá tengo datos que faltan por completar en los buffers
*/
void restart_binary(struct epoll_event* evlist);

#endif /*__BINARY_H__*/
