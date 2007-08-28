#ifndef __OPTIONS_SCREEN__
	#define __OPTIONS_SCREEN__

	#include <list>
	using std::list;

	class OptionsScreen 
	{
		public:
			OptionsScreen();
			virtual ~OptionsScreen(){};
			
			virtual void UpdateOptionsData();

			virtual void Activate();

			virtual void InputHandler(int iButtonMask);
			
			virtual int  Start_Network(int iNewProfile = -1);

			virtual void LoadFromConfig();
			virtual void SaveToConfigFile();

			
			#define MAX_OPTION_LENGTH 60
			#define MAX_NUM_OF_OPTIONS 20
			
			/** Options screen */
			struct Options
			{
				int	 Id;
				char strName[MAX_OPTION_LENGTH];
				char *strStates[MAX_NUM_OF_OPTIONS];
				int  iActiveState;		/** indicates the currently active state -- user pressed X on this state */
				int  iSelectedState;	/** selection 'box' around option; not active until user presses X */
				int  iNumberOfStates;
			};

			bool m_Dirty;
			list<Options> m_OptionsList;
			list<Options>::iterator m_CurrentOptionIterator;

		protected:
			virtual void OnOptionActivation();
		
		private:
			int  Stop_Network();
			int  GetCurrentNetworkProfile() { return m_iNetworkProfile; }
		
		protected:
			list<int> listints;
			int m_iNetworkProfile;
			bool m_WifiAutoStart, m_USBAutoStart;
			int  iWifiProfileOptionMap[MAX_NUM_OF_OPTIONS];
			//CPSPUSBStorage *m_USBStorage;

	};

#endif
