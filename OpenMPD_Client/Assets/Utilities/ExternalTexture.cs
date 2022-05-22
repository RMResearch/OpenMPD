﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;
using System.Runtime.InteropServices;

public class ExternalTexture : MonoBehaviour
{
    Texture2D _texture;
    CommandBuffer _command;

    [DllImport("OpenMPD.dll")]
    static extern System.IntPtr GetTextureUpdateCallback(); 

    void Start()
    {
        _command = new CommandBuffer();
        _texture = new Texture2D(64, 64, TextureFormat.RGBA32, false);
        _texture.wrapMode = TextureWrapMode.Clamp;

        // Set the texture to the renderer with using a property block.
        var prop = new MaterialPropertyBlock();
        prop.SetTexture("_MainTex", _texture);
        GetComponent<Renderer>().SetPropertyBlock(prop);
    }

    void OnDestroy()
    {
        _command.Dispose();
        Destroy(_texture);
    }

    void Update()
    {
        // Request texture update via the command buffer.
        _command.IssuePluginCustomTextureUpdateV2(
            /*OpenMPD_Wrapper.GetTextureUpdateCallback(), _texture, (uint)(Time.time * 60)*/
            GetTextureUpdateCallback(), _texture, (uint)(Time.time * 60)
        );

        Graphics.ExecuteCommandBuffer(_command);
        _command.Clear();

        // Rotation
        transform.eulerAngles = new Vector3(10, 20, 30) * Time.time;
    }
}
