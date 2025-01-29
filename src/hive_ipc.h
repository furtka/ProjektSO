
#ifndef HIVE_IPC_H
#define HIVE_IPC_H

// TODO: consider moving to types.h
#define RESULT int
#define SUCCESS 0
#define FAILURE -1
#define INTERRUPTED -2
#define handle_failure(x)   \
    switch (x)              \
    {                       \
    case INTERRUPTED:       \
        return INTERRUPTED; \
    case FAILURE:           \
        return FAILURE;     \
    default:                \
        break;              \
    }

#define handle_result(x, y) \
    x = y;                  \
    handle_failure(x);

#define handle_result_as(x, y) \
    RESULT x = y;              \
    handle_failure(x);

#define GATES_NUMBER 2

/**
 * Initializes the message queue that is used for communication between the hive
 * and the bees that are trying to enter or leave the hive.
 *
 * If this function is called multiple times, the message queue will be initialized
 * multiple times.
 *
 * Each call to this function should be matched with a call to
 * cleanup_gate_message_queue.
 */
RESULT initialize_gate_message_queue();

/**
 * Cleans up the message queue that is used for communication between the hive
 * and initialized by the initialize_gate_message_queue function.
 */
RESULT cleanup_gate_message_queue();

/**
 * Awaits for a bee to request to leave the hive.
 *
 * @return int bee_id of the bee that requested to leave the hive.
 *         Negative value if an error occurred.
 */
int await_bee_request_leave();

/**
 * Await for a bee to request to enter the hive.
 *
 * @return int bee_id of the bee that requested to enter the hive.
 *         Negative value if an error occurred.
 */
int await_bee_request_enter();

/**
 * Sends a message to the bee given by the first parameter that it is allowed to
 * leave the hive using the gate given by the second parameter.
 *
 * Assumes that the bee requested to leave the hive earlier and the message was
 * received by await_bee_request_leave function.
 *
 * @param bee_id Id of the bee that is allowed to leave the hive.
 * @param gate_id Id of the gate that the bee is allowed to use to leave the hive.
 *
 * @return SUCCESS if the message was successfully sent,
 *         FAILURE otherwise. See RESULT values for details.
 */
RESULT send_bee_allow_leave_message(int bee_id, int gate_id);

/**
 * Sends a message to the bee given by the first parameter that it is allowed to
 * enter the hive using gate given by the second parameter.
 *
 * Assumes that the bee requested to enter the hive earlier and the message was
 * received by await_bee_request_enter function.
 *
 * @param bee_id Id of the bee that is allowed to enter the hive.
 * @param gate_id Id of the gate that the bee is allowed to use to enter the hive.
 *
 * @return SUCCESS if the message was successfully sent,
 *         FAILURE otherwise. See RESULT values for details.
 */
RESULT send_bee_allow_enter_message(int bee_id, int gate_id);

/**
 * Awaits for a confirmation message from a bee that it has left the hive through
 * the gate given by the parameter.
 *
 * @param gate_id Id of the gate that the bee has left through.
 *
 * @return SUCCESS if the message was successfully received,
 *         FAILURE otherwise. See RESULT values for details.
 */
RESULT await_bee_leave_confirmation(int gate_id);

/**
 * Awaits for a confirmation message from a bee that it has entered the hive through
 * the gate given by the parameter.
 *
 * @param gate_id Id of the gate that the bee has entered through.
 *
 * @return SUCCESS if the message was successfully received,
 *         FAILURE otherwise. See RESULT values for details.
 */
RESULT await_bee_enter_confirmation(int gate_id);

/**
 * Sends a message to use the gate to enter the hive by the bee given by the
 * parameter.
 *
 * @param bee_id Id of the bee that is requesting to enter the hive.
 *
 * @return SUCCESS if the message was successfully sent,
 *         FAILURE otherwise.
 */
RESULT request_enter(int bee_id);

/**
 * Awaits for a message that the bee given by the parameter is allowed to use the
 * gate to enter the hive.
 *
 * @param bee_id Id of the bee that is requesting to enter the hive.
 *
 * @return int Id of the gate that the bee is allowed to use to enter the hive.
 *         Negative value if an error occurred.
 */
int await_use_gate_allowance(int bee_id);

/**
 * Sends a message to use the gate to leave the hive by the bee given by the
 * parameter.
 *
 * @param bee_id Id of the bee that is requesting to leave the hive.
 *
 * @return SUCCESS if the message was successfully sent,
 *         FAILURE otherwise. See RESULT values for details.
 */
RESULT send_enter_confirmation(int gate_id);

/**
 * Sends a message to use the gate to leave the hive by the bee given by the
 * parameter.
 *
 * @param bee_id Id of the bee that is requesting to leave the hive.
 *
 * @return SUCCESS if the message was successfully sent,
 *         FAILURE otherwise. See RESULT values for details.
 */
RESULT request_leave(int bee_id);

/**
 * Sends a message to confirm that the bee given by the parameter has left the hive.
 *
 * @param gate_id Id of the gate that the bee has left through.
 *
 * @return SUCCESS if the message was successfully sent,
 *         FAILURE otherwise. See RESULT values for details.
 */
RESULT send_leave_confirmation(int gate_id);

/**
 * Awaits for a message from the queen process with give birth request.
 * 
 * @return SUCCESS if the message was successfully received,
 *         FAILURE otherwise. See RESULT values for details.
 */
RESULT await_queen_birth_request();

/**
 * Sends a message to the queen process that it is allowed to give birth.
 * 
 * @return SUCCESS if the message was successfully sent,
 *         FAILURE otherwise. See RESULT values for details.
 */
RESULT send_queen_birth_allowance();

/**
 * Awaits for a message from the queen process that the queen bee was born.
 * 
 * @return int PID of the new bee process.
 *         Negative value if an error occurred. See RESULT values for details.
 */
int await_queen_birth_confirmation();

/**
 * Sends a message to the hive by queen process that with request permission
 * to give birth.
 * 
 * @return SUCCESS if the message was successfully sent,
 *         FAILURE otherwise. See RESULT values for details.
 */
RESULT request_queen_bee_born();

/**
 * Awaits for a message from the hive by queen process that it is allowed to give birth.
 * 
 * @return SUCCESS if the message was successfully received,
 *         FAILURE otherwise. See RESULT values for details.
 */
RESULT await_queen_birth_allowance();

/**
 * Sends a message to the hive by queen process that the queen bee was born.
 * 
 * @param PID of the new bee process.
 * 
 * @return RESULT SUCCESS if the message was successfully sent,
 *         FAILURE otherwise. See RESULT values for details.
 */
RESULT send_queen_birth_confirmation(int);

#endif