#ifndef LPAI_BT_AUDIO_TONE_H
#define LPAI_BT_AUDIO_TONE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h> 
#include "qapi_audio_service.h"
#include "qapi_audio_service_defs.h"

typedef qapi_as_client_cb_event qapi_at_client_cb_event;
//typedef qapi_as_client_cb_event_getsupported_effects_t qapi_get_tone_id;


/** @enum pb_session_state
* Enum for playback session state, it is used to know the current state of the session.
*
* @param None initial state of the session
* @param Opening session state should be changed to Opening as qapi_audio_session_open() invoked
* @param Opened Once session open cfm received, session state set to be Opened.
* @param Audio_playing session state should be changed to Audio_playing when qapi_audio_session_start() invoked. 
* @param Closed should be update as session closed.
*/
typedef enum{
None = 0,
Opening,
Opened,
Audio_playing,                 
}pb_session_state;


/**
 * @enum qapi_bt_audiotone_status_code_t
 */
typedef enum{
    QAPI_BT_ERROR_TYPE_SUCCESS = QAPI_AS_ERROR_TYPE_SUCCESS,                 /**< App Registration was Successful */
    QAPI_BT_ERROR_TYPE_FAIL,                 /**< App Registration Failed */
}qapi_bt_audiotone_status_code_t;

/**
 * @struct qapi_get_tone_id_t
 * @brief Structure to hold a list of tone IDs retrieved from the audio services.
 *
 * This structure is used to store multiple tone id along with the count
 * of tones available or retrieved.
 */

typedef struct qapi_get_tone_id{
    uint16_t num_tones;                 /**< Number of tones retrieved.*/
    int32_t tone_id[QAPI_AS_MAX_TONES]; /**< Array to store the tone ids.*/
}qapi_get_tone_id_t;


/**
 * @struct audio_tone_info_t
 * @brief Structure to hold information related to an audio tone session.
 */

typedef struct audio_tone_info
{
    bool enable_audio_tone;             /**< Flag to enable or disable the audio tone. */
    int32_t tone_id;                    /**< Unique identifier for the tone. */
    int32_t session_id;                 /**< Identifier for the session associated with the tone.*/
    int32_t session_cookie;             /**< Cookie value used to validate or track the session.*/
    pb_session_state session_state;     /**< Current state of the session */
    uint32_t pending_notification;      /**< Indicating pending notifications count for the session. */
    bool    stop_in_progress;           /**< Flag to handle disable sequence. */
}audio_tone_info_t;


/**
 * @brief Retrieves the list of supported tone IDs from the BT audio service.
 *
 * This function queries the audio service for all tone ids that are
 * currently available. These tone IDs can be used for tone playback or configuration
 * in BT audio sessions.
 *
 * @return QAPI_BT_ERROR_TYPE_SUCCESS if able to fetch the tones
 */
qapi_bt_audiotone_status_code_t qapi_bt_get_supported_tone_ids();


/**
 * @brief Configures the Bluetooth audio tone playback.
 *
 * This function enables or disables the playback of a specific audio tone
 * over the BT audio service. It is used to enable the tones
 * such as alerts or notifications with the given tone ids.
 *
 * @param[in] enable   Boolean flag to enable or disable the tone.
 * @param[in] tone_id  Identifier of the tone to be configured.
 *
 * @return void
 */
qapi_bt_audiotone_status_code_t qapi_bt_configure_audio_tone(bool enable, int32_t tone_id);

/**
 * @brief Global callback handler for audio client events.
 *
 * This function is invoked when an event is received from the audio service.
 * It handles various types of events and processes associated data accordingly.
 *
 * @param[in] evt            The event type received from the audio service.
 * @param[in] data           Pointer to event-specific data. The actual type depends on the event.
 * @param[in] client_cookie  A user-defined cookie value to help identify the client context.
 *
 * @return void
 */
void audio_global_client_cb(qapi_at_client_cb_event evt, void* data, int32_t client_cookie);


/**
 * @brief Opens a new audio tone session.
 *
 * This function initializes and opens a playback session for audio tone to play.
 * It sets up the necessary resources and context required to manage tone operations
 * such as device type, playback request type and the tone id for the session.
 *
 * @return void
 */
void audio_tone_session_open();

/**
 * @brief Starts playback of the configured audio tone.
 *
 * This function initiates the playback of an audio tone in an active session.
 * It assumes that the tone has already been configured and the session is open.
 * Useful for triggering alerts or notifications.
 *
 * @return void
 */
void audio_tone_start_play();


/**
 * @brief Stops the currently playing audio tone.
 *
 * This function halts the playback of an active audio tone session.
 * It is typically used to terminate tone playback prematurely or after completion.
 *
 * @note This function assumes that a tone is currently playing and that the session is active.
 *
 * @return QAPI_BT_ERROR_TYPE_SUCCESS, if audio tone stops play.
 */
qapi_bt_audiotone_status_code_t qapi_audio_tone_stop_play();


/**
 * @brief Closes the current audio tone session.
 *
 * This function releases resources and terminates the session associated with audio tone playback.
 * It should be called after tone playback is stopped to ensure proper cleanup.
 *
 * @return QAPI_BT_ERROR_TYPE_SUCCESS, if audio session closes.
 */
qapi_bt_audiotone_status_code_t audio_tone_session_close();

/**
 * @brief Callback function for audio session events.
 *
 * This function is invoked when an event related to an audio session occurs.
 * It provides detailed context including session identifiers and user-defined cookies,
 * allowing the application to handle session-specific logic.
 *
 * @param[in] evt             The event type received from the audio services.
 * @param[in] data            Pointer to event-specific data. The actual type depends on the event.
 * @param[in] client_cookie   A user-defined cookie value to help identify the client context.
 * @param[in] session_id      Identifier for the audio session associated with the event.
 * @param[in] session_cookie  Cookie value used to validate or track the session.
 *
 * @return void
 */
void qapi_audio_session_cb(qapi_at_client_cb_event evt, void* data, int32_t client_cookie, int32_t session_id, int32_t session_cookie);

#endif /*LPAI_BT_AUDIO_TONE_H */