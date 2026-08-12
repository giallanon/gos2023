#ifndef _gosFSWatcherInterface_h_
#define _gosFSWatcherInterface_h_
#include "gosEnumAndDefine.h"

namespace gos
{
	/***************************************************************
	* @brief	FSWatcherInterface
	* 
	* A causa delle sostanziali differenze tra i vari OS, questa classe e' parziale.
	* La specifica implementazione deve essere fatta per ogni OS
	*/
	class FSWatcherInterface
	{
	public:
		enum class eWhat : u8
		{
			unknown = 0,
			created = 1,
			modified = 2,
			renamed = 3,
			deleted = 4,
		};

	public:
						FSWatcherInterface()		{ }
		virtual			~FSWatcherInterface() 		{ }

		virtual void	begin () = 0;

		/**
		 * @brief	add_folder
		 * 			Aggiunge <folder_path> all'elenco dei folder sotto controllo.
		 * 			Aggiungere N volte lo stesso folder non e' un problema
		 */
		virtual void	add_folder (const char *folder_path) = 0;
		
		virtual bool	end() = 0;


		/**
		 * @brief	wait
		 * 			Attende per un massimo di <timeout_msec> e ritorna il numero di eventi che sono capitati.
		 * 			Se <timeout_msec> == u32MAX, allora attende per sempre fino a che non capita almeno un evento */
		virtual u32		wait (u32 timeout_msec) = 0;


		virtual u32		event__get_num () const = 0;
		virtual eWhat	event__get_what (u32 i) const = 0;

		virtual bool	event_is_a_dir (u32 i) const = 0;

		/**
		 * @brief	event__get_fullpath
		 * 			Ritorna il full pathname del file che e' stato notificato */
		virtual void	event__get_fullpath (u32 i, char *out__fullpath, u32 sizeof_out) const = 0;

		/**
		 * @brief	event__get_renamed_fullpath
		 * 			Se event__get_what() ritorna l'evento "renamed", allora questa fn ritorna il nuovo nome del file */
		virtual void	event__get_renamed_fullpath (u32 i, char *out__fullpath, u32 sizeof_out) const = 0;
		
	};

} //namespace ogs

#endif //_gosFSWatcherInterface_h_