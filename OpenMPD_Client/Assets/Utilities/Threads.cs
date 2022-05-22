
/*
 *This script just creats 2 threads that draw its number under a GUI label.
 * First time the button is used its call the start() of its respective thread.
 * Second time the button is used it pause/play the Thread.
 * To make it work, you just need to add this script to any object in the scene!
 *
 * Made by Ivan S. Cavalheiro
 * Date: 10/05/2012
 * Mail: ivanscavalheiro@hotmail.com
 * Script vers.: 1.2
*/

using UnityEngine;
using System.Collections;
using System.Threading;

public class Threads : MonoBehaviour
{
    #region Public data
    public float timeWaiting = 5000000.0f;
    public string labelInitialText = "I`m the console here!";
    #endregion

    #region Private data
    private string _label;
    private Thread _t1;
    private Thread _t2;
    private bool _t1Paused = false;
    private bool _t2Paused = false;
    private Mutex _mutex = new Mutex();
    #endregion

    #region Start
    void Start()
    {
        _label = labelInitialText;
        _t1 = new Thread(_func1);
        _t2 = new Thread(_func2);
    }
    #endregion

    #region Threads
    private void _func1()
    {
        if (_label == labelInitialText)
            _label = "";

        while (true)
        {
            _mutex.WaitOne();
            _label += 1;
            _mutex.ReleaseMutex();

            for (int i = 0; i < timeWaiting; i++)
                while (_t1Paused) { }
        }
    }

    private void _func2()
    {
        if (_label == labelInitialText)
            _label = "";

        while (true)
        {
            _mutex.WaitOne();
            _label += 2;
            _mutex.ReleaseMutex();

            for (int i = 0; i < timeWaiting; i++)
                while (_t2Paused) { }
        }
    }
    #endregion

    #region OnGUI
    void OnGUI()
    {
        //--> Label that servers as a "console"
        GUI.Label(new Rect(0, 0, 500, 500), _label);

        //--> Button for thread 1
        if (GUI.Button(new Rect(50, 50, 100, 50), "Thread T1"))
        {
            if (!_t1.IsAlive)
                _t1.Start();
            else
                _t1Paused = !_t1Paused;
        }

        //--> Button for thread 2
        if (GUI.Button(new Rect(50, 120, 100, 50), "Thread T2"))
        {
            if (!_t2.IsAlive)
                _t2.Start();
            else
                _t2Paused = !_t2Paused;
        }
    }
    #endregion
}