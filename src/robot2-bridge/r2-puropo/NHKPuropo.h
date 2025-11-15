/**
 * @file puropo.h
 * @author 五十嵐　幸多 (kotakota925ika@gmail.com)
 * @brief プロポの処理
 * @version 0.1
 * @date 2024-08-09
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef NHK2024A_PUROPO_H
#define NHK2024A_PUROPO_H

#include <mbed.h>
#include <puropo.h>

class NHK_Puropo
{
public:
    /**
     * @brief コンストラクタ
     *
     */
    NHK_Puropo(PinName tx, PinName rx) : puropo(tx, rx) {}

    /**
     * @brief セットアップ関数
     *
     */
    void setup()
    {
        puropo.start();
    }

    /**
     * @brief ループ処理
     *
     * @return true 受信成功
     * @return false 受信失敗
     */
    bool update()
    {
        ok = puropo.is_ok();
        return ok;
    }

    /**
     * @brief 受信が成功したかどうか
     *
     * @return true 受信成功
     * @return false 受信失敗
     */
    bool is_ok()
    {
        return ok;
    }

    float get(int ch)
    {
        return puropo.get(ch);
    }

    /**
     * @brief プリントデバッグ関数
     *
     */
    void print_debug()
    {
        // printf("ch1:%d, ", puropo.get_ch(1));
        // printf("ch2:%d, ", puropo.get_ch(2));
        // printf("ch3:%d, ", puropo.get_ch(3));
        // printf("ch4:%d, ", puropo.get_ch(4));
        // printf("ch1:%d, ", puropo.get(1));
        // printf("ch2:%d, ", puropo.get(2));
        // printf("ch3:%d, ", puropo.get(3));
        // printf("ch4:%d, ", puropo.get(4));
        printf("puropo:");
        for (int i = 1; i <= 10; i++)
            printf("%0.2f ", this->get(i));
        printf("|");
    }

private:
    Puropo puropo;
    bool ok = false;
};

#endif