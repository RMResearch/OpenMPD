using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

public abstract class ArrayBuilderNode<T> : DataNode {
    
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited, dynamicPortList = true)] public T[] inputObjects = new T[0];
    [Output(ShowBackingValue.Never, ConnectionType.Override, TypeConstraint.Inherited)] public T[] outputArray = new T[0];

    public override object GetValue(NodePort port)
    {
        return outputArray;
    }


    public override void UpdatePorts()
    {
        base.UpdatePorts();

        if(outputArray.Length != inputObjects.Length){
            T[] prevOutput = outputArray;
            outputArray = new T[inputObjects.Length];
            System.Array.Copy(prevOutput, outputArray, Mathf.Min(prevOutput.Length, inputObjects.Length));
        }

        for (int i = 0; i < inputObjects.Length; i++)
        {
            NodePort objectPort = GetPort("inputObjects " + i.ToString());
            if (objectPort != null)
            {
                object objectPortValue = objectPort.GetInputValue();
                if (objectPortValue != null)
                {
                    T castedValue = default(T);
                    try
                    {
                        castedValue = (T)objectPortValue;
                    }catch(System.Exception)
                    {
                        Debug.LogError("Type missmatch: expected <" + typeof(T) + ">, received <" + objectPortValue.GetType() + ">");
                    }
                    inputObjects[i] = castedValue;
                    outputArray[i] = castedValue;
                }
                else
                {
                    outputArray[i] = inputObjects[i];
                }
            }
            else
            {
                inputObjects[i] = default(T);
                outputArray[i] = default(T);
            }
        }
    }

    public override void OnCreateConnection(NodePort from, NodePort to) {
        base.OnCreateConnection(from, to);

        if(outputArray.Length != inputObjects.Length){
            T[] prevOutput = outputArray;
            outputArray = new T[inputObjects.Length];
            System.Array.Copy(prevOutput, outputArray, Mathf.Max(prevOutput.Length, inputObjects.Length));
        }

        for(int i = 0; i < inputObjects.Length; i++){
            NodePort objectPort = GetPort("inputObjects " + i.ToString());
            if (objectPort != null)
            {
                object objectPortValue = objectPort.GetInputValue();
                if (objectPortValue != null)
                {
                    T castedValue = default(T);
                    try
                    {
                        castedValue = (T)objectPortValue;
                    }
                    catch (System.Exception)
                    {
                        Debug.LogError("Type missmatch: expected <" + typeof(T) + ">, received <" + objectPortValue.GetType() + ">");
                    }
                    inputObjects[i] = castedValue;
                    outputArray[i] = castedValue;
                }
                else
                {
                    outputArray[i] = inputObjects[i];
                }
            }
            else
            {
                inputObjects[i] = default(T);
                outputArray[i] = default(T);
            }
        }
    }

    public override void OnRemoveConnection(NodePort port)
    {
        base.OnRemoveConnection(port);

        if(outputArray.Length != inputObjects.Length){
            T[] prevOutput = outputArray;
            outputArray = new T[inputObjects.Length];
            System.Array.Copy(prevOutput, outputArray, Mathf.Max(prevOutput.Length, inputObjects.Length));
        }

        for (int i = 0; i < inputObjects.Length; i++)
        {
            NodePort objectPort = GetPort("inputObjects " + i.ToString());
            if (objectPort != null)
            {
                object objectPortValue = objectPort.GetInputValue();
                if (objectPortValue != null)
                {
                    T castedValue = default(T);
                    try
                    {
                        castedValue = (T)objectPortValue;
                    }
                    catch (System.Exception)
                    {
                        Debug.LogError("Type missmatch: expected <" + typeof(T) + ">, received <" + objectPortValue.GetType() + ">");
                    }
                    inputObjects[i] = castedValue;
                    outputArray[i] = castedValue;
                }
                else
                {
                    outputArray[i] = inputObjects[i];
                }
            }
            else
            {
                inputObjects[i] = default(T);
                outputArray[i] = default(T);
            }
        }
    }
}