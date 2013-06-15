#pragma once


namespace soft {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

		using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::IO::Ports;

	 using namespace System;
	 using namespace System ::IO;
	/// <summary>
	/// Summary for Form1
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		Form1(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
		}


	protected: 

	private: System::IO::Ports::SerialPort^  serialPort;


	private: System::Windows::Forms::Timer^  timer1;
	private: System::Windows::Forms::ListBox^  lcodelist;
	private: System::Windows::Forms::Button^  loadFileButton;
	private: System::Windows::Forms::OpenFileDialog^  openFile;
	private: System::Windows::Forms::Label^  info;
	private: System::Windows::Forms::Label^  size;
	private: System::Windows::Forms::Timer^  sendTimer;

	private: System::ComponentModel::IContainer^  components;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			this->serialPort = (gcnew System::IO::Ports::SerialPort(this->components));
			this->timer1 = (gcnew System::Windows::Forms::Timer(this->components));
			this->lcodelist = (gcnew System::Windows::Forms::ListBox());
			this->loadFileButton = (gcnew System::Windows::Forms::Button());
			this->openFile = (gcnew System::Windows::Forms::OpenFileDialog());
			this->info = (gcnew System::Windows::Forms::Label());
			this->size = (gcnew System::Windows::Forms::Label());
			this->sendTimer = (gcnew System::Windows::Forms::Timer(this->components));
			this->SuspendLayout();
			// 
			// serialPort
			// 
			this->serialPort->BaudRate = 19200;
			this->serialPort->PortName = L"COM2";
			this->serialPort->ReadBufferSize = 256;
			this->serialPort->ReadTimeout = 16;
			this->serialPort->WriteBufferSize = 256;
			this->serialPort->WriteTimeout = 16;
			// 
			// timer1
			// 
			this->timer1->Enabled = true;
			this->timer1->Interval = 1;
			this->timer1->Tick += gcnew System::EventHandler(this, &Form1::timer1_Tick);
			// 
			// lcodelist
			// 
			this->lcodelist->FormattingEnabled = true;
			this->lcodelist->Location = System::Drawing::Point(12, 12);
			this->lcodelist->Name = L"lcodelist";
			this->lcodelist->Size = System::Drawing::Size(273, 498);
			this->lcodelist->TabIndex = 0;
			// 
			// loadFileButton
			// 
			this->loadFileButton->Location = System::Drawing::Point(291, 12);
			this->loadFileButton->Name = L"loadFileButton";
			this->loadFileButton->Size = System::Drawing::Size(128, 34);
			this->loadFileButton->TabIndex = 1;
			this->loadFileButton->Text = L"Load lcode file";
			this->loadFileButton->UseVisualStyleBackColor = true;
			this->loadFileButton->Click += gcnew System::EventHandler(this, &Form1::loadFileButton_Click);
			// 
			// openFile
			// 
			this->openFile->FileName = L"openFile";
			// 
			// info
			// 
			this->info->AutoSize = true;
			this->info->Location = System::Drawing::Point(301, 84);
			this->info->Name = L"info";
			this->info->Size = System::Drawing::Size(15, 13);
			this->info->TabIndex = 2;
			this->info->Text = L"l0";
			// 
			// size
			// 
			this->size->AutoSize = true;
			this->size->Location = System::Drawing::Point(311, 222);
			this->size->Name = L"size";
			this->size->Size = System::Drawing::Size(35, 13);
			this->size->TabIndex = 3;
			this->size->Text = L"label1";
			// 
			// sendTimer
			// 
			this->sendTimer->Interval = 700;
			this->sendTimer->Tick += gcnew System::EventHandler(this, &Form1::sendTimer_Tick);
			// 
			// Form1
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(438, 527);
			this->Controls->Add(this->size);
			this->Controls->Add(this->info);
			this->Controls->Add(this->loadFileButton);
			this->Controls->Add(this->lcodelist);
			this->Name = L"Form1";
			this->Text = L"Form1";
			this->Load += gcnew System::EventHandler(this, &Form1::Form1_Load);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
		
		String ^received;
		int size_in;
	private: System::Void Form1_Load(System::Object^  sender, System::EventArgs^  e) 
			 {
				serialPort->Open();
				serialPort->DataReceived += gcnew System::IO::Ports::SerialDataReceivedEventHandler(this, &Form1::DataReceivedHandler);
				size_in = 0;
				received = " ";
			 }
private: System::Void DataReceivedHandler( Object^ sender, SerialDataReceivedEventArgs^ e)
		 {
			SerialPort^ sp = (SerialPort^)sender;
			received += sp->ReadExisting();
		 }
		 
private: System::Void timer1_Tick(System::Object^  sender, System::EventArgs^  e) 
		 {
			try 
			{			
				 if( received->Length == 0 )
					 return;

				array<wchar_t> ^delim = gcnew array<wchar_t>( 2 );

				delim[ 0 ] = Convert::ToChar( " " );
				delim[ 1 ] = Convert::ToChar( ";" );

				array<String^> ^arr = received->Split( delim );

				bool readarg = 0;

				for each ( String^ str in arr )
				{
					if( readarg )
					{
						size->Text = str;

						size_in = Convert::ToInt32( str );

						break;
					}
					else
					{
						if( str->Length )
						{
							wchar_t cmd= str[ 0 ];

							if( cmd == L'S' )
							{
								readarg = 1; 
							}
						}
					}
				}

				info->Text = received;
				received = "";
			} 
			catch( NullReferenceException ^ )
			{
				received = "";
			}
				
		 }

private: System::Void loadFileButton_Click(System::Object^  sender, System::EventArgs^  e) 
		 {
			 System::Windows::Forms::DialogResult result = openFile->ShowDialog();

			 if( result == System::Windows::Forms::DialogResult::OK )
			 {
				StreamReader ^ sr = gcnew StreamReader( openFile->FileName );

				String ^ line;

				while( (line = sr->ReadLine()) ) 
				{
					lcodelist->Items->Add( line );
				}			 				
			 };	

			 sendTimer->Enabled = true;
		 }
private: System::Void sendTimer_Tick(System::Object^  sender, System::EventArgs^  e) 
		 {
			 static bool stop = 0;
			 static int cmdNum = 0;

			 if( !stop )
			 {
				 if( size_in < 64 )
				 {
					 if( lcodelist->Items->Count != 0 )
					 {
						 String ^ out = (Convert::ToString( lcodelist->Items[ 0 ] ));

						 serialPort->Write( out);

						lcodelist->Items->RemoveAt( 0 );
					 }
				 }
			 }
		 }
};
}

