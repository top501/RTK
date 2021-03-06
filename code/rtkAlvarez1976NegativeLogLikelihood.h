/*=========================================================================
 *
 *  Copyright RTK Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#ifndef rtkAlvarez1976NegativeLogLikelihood_h
#define rtkAlvarez1976NegativeLogLikelihood_h

#include "rtkProjectionsDecompositionNegativeLogLikelihood.h"

#include <itkVectorImage.h>
#include <itkVariableLengthVector.h>
#include <itkVariableSizeMatrix.h>

namespace rtk
{
  /** \class rtkAlvarez1976NegativeLogLikelihood
   * \brief Cost function from the Alvarez 1976 PMB paper
   *
   * See the reference paper: "Energy-selective reconstructions in
   * X-Ray Computerized Tomography", Alvarez and Macovski, PMB 1976
   *
   * \author Cyril Mory
   *
   * \ingroup ReconstructionAlgorithm
   */

// We have to define the cost function first
class Alvarez1976NegativeLogLikelihood : public rtk::ProjectionsDecompositionNegativeLogLikelihood
{
public:

  typedef Alvarez1976NegativeLogLikelihood                      Self;
  typedef rtk::ProjectionsDecompositionNegativeLogLikelihood    Superclass;
  typedef itk::SmartPointer<Self>                               Pointer;
  typedef itk::SmartPointer<const Self>                         ConstPointer;
  itkNewMacro( Self );
  itkTypeMacro( Alvarez1976NegativeLogLikelihood, rtk::ProjectionsDecompositionNegativeLogLikelihood );

  typedef Superclass::ParametersType          ParametersType;
  typedef Superclass::DerivativeType          DerivativeType;
  typedef Superclass::MeasureType             MeasureType;

  typedef itk::VariableSizeMatrix<float>      DetectorResponseType;
  typedef itk::VariableSizeMatrix<float>      MaterialAttenuationsType;
  typedef itk::VariableLengthVector<float>    MeasuredDataType;
  typedef itk::VariableLengthVector<float>    IncidentSpectrumType;

  // Constructor
  Alvarez1976NegativeLogLikelihood()
  {
  }

  // Destructor
  ~Alvarez1976NegativeLogLikelihood()
  {
  }

  vnl_vector<float> ForwardModel(const ParametersType & lineIntegrals) const
  {
  // Variable length vector and variable size matrix cannot be used in linear algebra operations
  // Get their vnl counterparts, which can
  vnl_vector<float> vnl_vec_he(GetAttenuatedIncidentSpectrum(m_HighEnergyIncidentSpectrum, lineIntegrals).GetDataPointer(),
                               GetAttenuatedIncidentSpectrum(m_HighEnergyIncidentSpectrum, lineIntegrals).GetSize());
  vnl_vector<float> vnl_vec_le(GetAttenuatedIncidentSpectrum(m_LowEnergyIncidentSpectrum, lineIntegrals).GetDataPointer(),
                               GetAttenuatedIncidentSpectrum(m_LowEnergyIncidentSpectrum, lineIntegrals).GetSize());

  // Apply detector response, getting the lambdas
  return (m_DetectorResponse.GetVnlMatrix() * vnl_vec_he + m_DetectorResponse.GetVnlMatrix() * vnl_vec_le);
  }

  itk::VariableLengthVector<float> GetAttenuatedIncidentSpectrum(IncidentSpectrumType incident, const ParametersType & lineIntegrals) const
  {
  // Solid angle of detector pixel, exposure time and mAs should already be
  // taken into account in the incident spectrum image

  if(m_NumberOfEnergies != incident.GetSize())
      itkGenericExceptionMacro(<< "Incident spectrum does not have the correct size")

  // Apply attenuation at each energy
  itk::VariableLengthVector<float> attenuatedIncidentSpectrum;
  attenuatedIncidentSpectrum.SetSize(m_NumberOfEnergies);
  attenuatedIncidentSpectrum.Fill(0);
  for (unsigned int e=0; e<m_NumberOfEnergies; e++)
    {
    float totalAttenuation = 0.;
    for (unsigned int m=0; m<2; m++)
      {
      totalAttenuation += lineIntegrals[m] * m_MaterialAttenuations[m][e];
      }

    attenuatedIncidentSpectrum[e] = incident[e] * std::exp(-totalAttenuation);
    }

  return attenuatedIncidentSpectrum;
  }

  // Main method
  MeasureType  GetValue( const ParametersType & parameters ) const ITK_OVERRIDE
  {
  // Forward model: compute the expected number of counts in each bin
  vnl_vector<float> lambdas = ForwardModel(parameters);

  // Compute the negative log likelihood from the lambdas
  MeasureType measure = 0;
  for (unsigned int i=0; i<2; i++)
    measure += lambdas[i] - std::log(lambdas[i]) * m_MeasuredData[i];
  return measure;
  }

  void GetDerivative( const ParametersType & lineIntegrals,
                      DerivativeType & derivatives ) const ITK_OVERRIDE
  {
  }

  itkSetMacro(HighEnergyIncidentSpectrum, IncidentSpectrumType)
  itkGetMacro(HighEnergyIncidentSpectrum, IncidentSpectrumType)

  itkSetMacro(LowEnergyIncidentSpectrum, IncidentSpectrumType)
  itkGetMacro(LowEnergyIncidentSpectrum, IncidentSpectrumType)

protected:
  IncidentSpectrumType        m_HighEnergyIncidentSpectrum;
  IncidentSpectrumType        m_LowEnergyIncidentSpectrum;


private:
  Alvarez1976NegativeLogLikelihood(const Self &); //purposely not implemented
  void operator = (const Self &); //purposely not implemented

};

}// namespace RTK

#endif
