#include <iostream>
#include <fstream>
using namespace std;

class Header
{
    uint8_t value[4];

public:
    Header() {}

    void parse(uint8_t* TS)
    {
        for (int i = 0; i < 4; i++)
        {
            value[i] = TS[i];
        }

        uint8_t Emask = 0x80;
        uint8_t Smask = 0x40;
        uint8_t Pmask = 0x20;

        uint8_t PIDmask = 0x1F;

        uint8_t TSCmask = 0xC0;
        uint8_t AFCmask = 0x30;
        uint8_t CCmask = 0x0F;

        uint16_t PID = value[1] & PIDmask;
        PID = PID << 8;

        cout << "TS: SB=" << (unsigned int)value[0] << " E=" << (bool)(value[1] & Emask) << " S=" << (bool)(value[1] & Smask) << " P=" << (bool)(value[1] & Pmask) << " PID=";
        if ((PID | value[2]) < 10)
            cout << "   ";
        else if ((PID | value[2]) < 100)
            cout << "  ";
        else if ((PID | value[2]) < 1000)
            cout << " ";
        cout << (PID | value[2]) << " TSC=" << ((value[3] & TSCmask) >> 6) << " AFC=" << ((value[3] & AFCmask) >> 4) << " CC=";
        if ((value[3] & CCmask) < 10)
            cout << " ";
        cout << (value[3] & CCmask) << " ";

    }

    bool ifAF()
    {
        uint8_t AFCmask = 0x30;

        if (((value[3] & AFCmask) >> 4) == 2 || ((value[3] & AFCmask) >> 4) == 3)
            return true;
        else
            return false;
    }

    bool ifSound()
    {
        uint8_t PIDmask = 0x1F;
        uint16_t PID = value[1] & PIDmask;
        PID = PID << 8;

        if ((PID | value[2]) == 136)
            return true;
        else
            return false;
    }

    uint8_t getCC()
    {
        uint8_t CCmask = 0x0F;
        return (value[3] & CCmask);
    }
};

class AdaptationField
{
    uint8_t* value;
    uint8_t AFlength;

public:
    AdaptationField() {}

    void parse(uint8_t* TS)
    {
        AFlength = TS[4] + 1;
        value = new uint8_t[AFlength];

        for (int i = 0; i < AFlength; i++)
        {
            value[i] = TS[i + 4];
        }

        uint8_t DCmask = 0x80;
        uint8_t RAmask = 0x40;
        uint8_t SPmask = 0x20;
        uint8_t PRmask = 0x10;
        uint8_t ORmask = 0x08;
        uint8_t SPFmask = 0x04;
        uint8_t TPmask = 0x02;
        uint8_t EXmask = 0x01;

        cout << "AF: L=" << (unsigned int)value[0] << " DC=" << (bool)(value[1] & DCmask) << " RA=" << (bool)(value[1] & RAmask) << " SP=" << (bool)(value[1] & SPmask) << " PR=" << (bool)(value[1] & PRmask) << " OR=" << (bool)(value[1] & ORmask) << " SP=" << (bool)(value[1] & SPFmask) << " TP=" << (bool)(value[1] & TPmask) << " EX=" << (bool)(value[1] & EXmask);
    }

    uint8_t getAFlength()
    {
        return AFlength;
    }

    ~AdaptationField()
    {
        delete value;
    }
};

class PES
{
    uint8_t* headerValue;
    uint16_t headerLength, packetLength, restPacketLength;
    uint8_t index;
    ofstream sound;
public:
    PES()
    {
        sound.open("PID136.mp2", ios::out | ios::binary);
    }

    void parse(uint8_t* TS, bool ifAF, uint8_t CC, uint8_t AFlength)
    {
        uint8_t PTSflagMask = 0x80;
        uint8_t DTSflagMask = 0x40;
        uint8_t ESCRflagMask = 0x20;
        uint8_t ESRateflagMask = 0x10;
        uint8_t AdCopyInfoflagMask = 0x04;
        uint8_t PESCRCflagMask = 0x02;
        uint8_t PESExflagMask = 0x01;
        uint8_t PESPrivateDataflagMask = 0x80;
        uint8_t PackHeaderFieldflagMask = 0x40;
        uint8_t ProgramPackSeqCounterflagMask = 0x20;
        uint8_t P_STDBufferflagMask = 0x10;
        uint8_t PESEx2flagMask = 0x01;
        uint64_t PTSDTSMask = 0x00000006FEFE;
        uint8_t restLength = 4;
        uint64_t PTS = 0, DTS = 0, temp = 0;

        if (ifAF)
        {
            index = 4 + AFlength;
            restLength += AFlength;
        }

        else
            index = 4;

        if ((TS[index] | TS[index + 1] | TS[index + 2]) == 0x000001 && CC == 0)
        {
            uint16_t streamID = (index + 3);
            headerLength = 6;
            if (streamID != 0xBC
                && streamID != 0xBE
                && streamID != 0xBF
                && streamID != 0xF0
                && streamID != 0xF1
                && streamID != 0xFF
                && streamID != 0xF2
                && streamID != 0xF8)
            {
                headerLength += 3;
                if ((bool)(TS[index + 7] & PTSflagMask))
                {
                    headerLength += 5;
                    int move = 32;
                    for (int i = 1; i < 6; i++)
                    {
                        temp = TS[index + 8 + i];
                        temp <<= move;
                        PTS = PTS | temp;
                        move -= 8;
                    }
                    PTS = PTS & PTSDTSMask;
                }
                if ((bool)(TS[index + 7] & DTSflagMask))
                {
                    headerLength += 5;
                    int move = 32;
                    for (int i = 1; i < 6; i++)
                    {
                        temp = TS[index + 13 + i];
                        temp <<= move;
                        DTS = DTS | temp;
                        move -= 8;
                    }
                    DTS = DTS & PTSDTSMask;
                }
                if ((bool)(TS[index + 7] & ESCRflagMask))
                    headerLength += 6;
                if ((bool)(TS[index + 7] & ESRateflagMask))
                    headerLength += 3;
                if ((bool)(TS[index + 7] & AdCopyInfoflagMask))
                    headerLength += 1;
                if ((bool)(TS[index + 7] & PESCRCflagMask))
                    headerLength += 2;
                if ((bool)(TS[index + 7] & PESExflagMask))
                {
                    headerLength += 1;
                    if ((bool)(TS[index + headerLength] & PESPrivateDataflagMask))
                        headerLength += 2;
                    if ((bool)(TS[index + headerLength] & PackHeaderFieldflagMask))
                        headerLength += 1;
                    if ((bool)(TS[index + headerLength] & ProgramPackSeqCounterflagMask))
                        headerLength += 2;
                    if ((bool)(TS[index + headerLength] & P_STDBufferflagMask))
                        headerLength += 2;
                    if ((bool)(TS[index + headerLength] & PESEx2flagMask))
                        headerLength += 2;
                }
            }

            headerValue = new uint8_t[headerLength];

            for (int i = 0; i < headerLength; i++)
            {
                headerValue[i] = TS[index + i];
            }

            packetLength = headerValue[4];
            packetLength <<= 8;
            packetLength = (packetLength | headerValue[5]);
            restPacketLength = packetLength;
            packetLength += 6;

            cout << std::endl << (unsigned int)index << std::endl;
            if (sound.is_open())
            {
                for (int i = 0; i < (188 - restLength - headerLength); i++)
                    sound << TS[index + headerLength + i];
            }

            cout << " Started ";
            cout << "PES: PSCP=" << 1 << " SID=" << (int)headerValue[3] << " L=" << packetLength - 6 << " PTS=" << PTS << " Time=" << DTS << "s";
            if ((188 - restLength) > packetLength)
                packetLength = 0;
            else
                packetLength -= (188 - restLength);
        }
        else if (packetLength > 0 && CC > 0 && CC < 15)
        {
            cout << " Continue ";
            if ((188 - restLength) > packetLength)
                packetLength = 0;
            else
                packetLength -= (188 - restLength);

            if (sound.is_open())
            {
                for (int i = 0; i < (188 - restLength); i++)
                    sound << TS[index + i];
            }
        }
        else
        {
            cout << " Finished " << "PES: PcktLen=" << restPacketLength + 6 << " HeadLen=" << headerLength << " DataLen=" << restPacketLength + 6 - headerLength;
            if (sound.is_open())
            {
                for (int i = 0; i < (188 - restLength); i++)
                    sound << TS[index + i];
            }
        }
    }

    ~PES()
    {
        delete headerValue;
        sound.close();
    }
};

void parseTS(const char* fileName)
{
    uint8_t TS[188];
    Header H;
    AdaptationField AF;
    PES P;

    int number = 1;
    int zeroes, i, tmp;

    ifstream file(fileName, ios::in | ios::binary);
    if (file.is_open())
    {
        int counter = 0;
        char c;
        while (file.get(c))
        {
            TS[counter] = (unsigned int)c;
            counter++;
            if (counter == 188)
            {
                i = 0;
                tmp = number;
                zeroes = 9;
                while (tmp / 10 > 0)
                {
                    if (zeroes == 0)
                        break;
                    zeroes--;
                    tmp /= 10;
                }

                while (i < zeroes)
                {
                    cout << 0;
                    i++;
                }
                cout << number << " ";
                H.parse(TS);
                if (H.ifAF())
                    AF.parse(TS);
                if (H.ifSound())
                    P.parse(TS, H.ifAF(), H.getCC(), AF.getAFlength());
                cout << endl;
                counter = 0;
                number++;
            }
        }
        file.close();
    }
    else
    {
        cout << "Nie udalo sie otworzyc pliku!" << endl;
    }
}

int main()
{
    parseTS("example_new.ts");
    return 0;
}
